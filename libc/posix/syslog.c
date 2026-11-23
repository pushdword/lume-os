/* SPDX-License-Identifier: BSD-3-Clause */
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

enum { TAG_BYTES = 32, RECORD_BYTES = 1024 };

static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
static int log_fd = -1;
static int log_options;
static int log_facility = LOG_USER;
static int log_mask = LOG_UPTO(LOG_DEBUG);
static char log_tag[TAG_BYTES + 1];

static void disconnect_log(void)
{
    if (log_fd >= 0)
        close(log_fd);
    log_fd = -1;
}

static bool connect_log(void)
{
    if (log_fd >= 0)
        return true;

    const struct sockaddr_un address = {.sun_family = AF_UNIX, .sun_path = "/dev/log"};
    int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
    if (fd < 0)
        return false;
    uid_t uid;
    gid_t gid;
    if (connect(fd, (const struct sockaddr *)&address, sizeof(address)) ||
        getpeereid(fd, &uid, &gid) || uid != 0)
    {
        close(fd);
        return false;
    }
    log_fd = fd;
    return true;
}

void openlog(const char *ident, int options, int facility)
{
    int saved = errno;
    pthread_mutex_lock(&log_lock);
    snprintf(log_tag, sizeof(log_tag), "%s", ident ? ident : "");
    log_options = options;
    if (facility > 0 && !(facility & ~LOG_FACMASK) && LOG_FAC(facility) < LOG_NFACILITIES)
        log_facility = facility;
    if (options & LOG_NDELAY)
        (void)connect_log();
    pthread_mutex_unlock(&log_lock);
    errno = saved;
}

void closelog(void)
{
    int saved = errno;
    pthread_mutex_lock(&log_lock);
    disconnect_log();
    log_tag[0] = 0;
    log_options = 0;
    log_facility = LOG_USER;
    pthread_mutex_unlock(&log_lock);
    errno = saved;
}

int setlogmask(int mask)
{
    pthread_mutex_lock(&log_lock);
    int previous = log_mask;
    if (mask)
        log_mask = mask;
    pthread_mutex_unlock(&log_lock);
    return previous;
}

/* Expand %m while preserving %% and any conversions consumed by vsnprintf. */
static int format_message(char *out, size_t capacity, const char *format, va_list arguments,
                          int error)
{
    size_t substitutions = 0;
    for (const char *p = format; *p; ++p)
        if (*p == '%' && p[1])
        {
            if (p[1] == 'm')
                ++substitutions;
            ++p;
        }
    if (!substitutions)
        return vsnprintf(out, capacity, format, arguments);

    const char *description = strerror(error);
    size_t escaped = 0;
    for (const char *p = description; *p; ++p)
        escaped += *p == '%' ? 2 : 1;
    size_t literal = strlen(format) - 2 * substitutions;
    if (escaped > (SIZE_MAX - literal - 1) / substitutions)
        return -1;
    char *expanded = malloc(literal + escaped * substitutions + 1);
    if (!expanded)
        return -1;

    char *at = expanded;
    for (const char *p = format; *p; ++p)
    {
        if (*p != '%' || !p[1])
        {
            *at++ = *p;
            continue;
        }
        if (*++p != 'm')
        {
            *at++ = '%';
            *at++ = *p;
            continue;
        }
        for (const char *word = description; *word; ++word)
        {
            if (*word == '%')
                *at++ = '%';
            *at++ = *word;
        }
    }
    *at = 0;
    int result = vsnprintf(out, capacity, expanded, arguments);
    free(expanded);
    return result;
}

static bool deliver_log(const char *record, size_t bytes)
{
    for (unsigned attempt = 0; attempt < 2 && connect_log(); ++attempt)
    {
        size_t sent = 0;
        while (sent < bytes)
        {
            ssize_t count = send(log_fd, record + sent, bytes - sent, MSG_NOSIGNAL);
            if (count < 0 && errno == EINTR)
                continue;
            if (count <= 0)
                break;
            sent += (size_t)count;
        }
        if (sent == bytes)
            return true;
        disconnect_log();
        if (sent)
            break;
    }
    return false;
}

static void write_line(int fd, const char *message, size_t bytes)
{
    while (bytes)
    {
        ssize_t count = write(fd, message, bytes);
        if (count < 0 && errno == EINTR)
            continue;
        if (count <= 0)
            return;
        message += count;
        bytes -= (size_t)count;
    }
}

void vsyslog(int priority, const char *format, va_list arguments)
{
    int saved = errno;
    int severity = LOG_PRI(priority), facility = priority & LOG_FACMASK;
    int options;
    char tag[TAG_BYTES + 1];
    pthread_mutex_lock(&log_lock);
    if (!(log_mask & LOG_MASK(severity)))
    {
        pthread_mutex_unlock(&log_lock);
        errno = saved;
        return;
    }
    options = log_options;
    if (!facility || LOG_FAC(facility) >= LOG_NFACILITIES)
        facility = log_facility;
    memcpy(tag, log_tag, sizeof(tag));
    pthread_mutex_unlock(&log_lock);

    if (!tag[0])
        snprintf(tag, sizeof(tag), "%s", getprogname());
    char stamp[20] = "";
    time_t now = time(NULL);
    struct tm clock;
    if (now != (time_t)-1 && gmtime_r(&now, &clock) &&
        !strftime(stamp, sizeof(stamp), "%b %e %H:%M:%S ", &clock))
        stamp[0] = 0;

    char record[RECORD_BYTES];
    char label[TAG_BYTES + 26];
    if (options & LOG_PID)
        snprintf(label, sizeof(label), "%s[%ld]", tag, (long)getpid());
    else
        snprintf(label, sizeof(label), "%s", tag);
    int prefix = snprintf(record, sizeof(record), "<%d>%s%s: ", facility | severity, stamp, label);
    if (prefix < 0 || (size_t)prefix >= sizeof(record) - 2)
    {
        errno = saved;
        return;
    }
    size_t tag_start = (size_t)prefix - strlen(label) - 2;
    size_t used = (size_t)prefix;
    size_t available = sizeof(record) - used - 1;
    int size = format_message(record + used, available, format, arguments, saved);
    if (size < 0)
    {
        errno = saved;
        return;
    }
    size_t message_bytes = (size_t)size < available ? (size_t)size : available - 1;
    for (size_t i = 0; i < message_bytes; ++i)
        if (!record[used + i])
            record[used + i] = '?';
    used += message_bytes;
    record[used] = 0;

    pthread_mutex_lock(&log_lock);
    bool delivered = deliver_log(record, used + 1);
    pthread_mutex_unlock(&log_lock);

    if (!used || record[used - 1] != '\n')
        record[used++] = '\n';
    if (options & LOG_PERROR)
        write_line(STDERR_FILENO, record + tag_start, used - tag_start);
    if (!delivered && (options & LOG_CONS))
    {
        int console = open("/dev/console", O_WRONLY | O_CLOEXEC | O_NOCTTY | O_NONBLOCK);
        if (console >= 0)
        {
            write_line(console, record + tag_start, used - tag_start);
            close(console);
        }
    }
    errno = saved;
}

void syslog(int priority, const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    vsyslog(priority, format, arguments);
    va_end(arguments);
}
