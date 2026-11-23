/* SPDX-License-Identifier: BSD-3-Clause */
#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <syslog.h>
#include <unistd.h>

static void send_variadic(int priority, const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    vsyslog(priority, format, arguments);
    va_end(arguments);
}

static int accept_logger(int listener)
{
    struct pollfd pending = {.fd = listener, .events = POLLIN};
    assert(poll(&pending, 1, 2000) == 1 && (pending.revents & POLLIN));
    int peer = accept(listener, NULL, NULL);
    assert(peer >= 0);
    return peer;
}

static void expect_record(int peer, int priority, const char *expected)
{
    char text[1024], prefix[16];
    size_t used = 0;
    for (;;)
    {
        struct pollfd pending = {.fd = peer, .events = POLLIN};
        assert(poll(&pending, 1, 2000) == 1 && (pending.revents & POLLIN));
        assert(used < sizeof(text));
        ssize_t bytes = recv(peer, text + used, sizeof(text) - used, 0);
        assert(bytes > 0);
        used += (size_t)bytes;
        if (memchr(text, 0, used))
            break;
    }
    assert(strlen(text) + 1 == used);
    assert(snprintf(prefix, sizeof(prefix), "<%d>", priority) > 0);
    assert(!strncmp(text, prefix, strlen(prefix)) && strstr(text, expected));
}

int main(void)
{
    struct sockaddr_un address = {.sun_family = AF_UNIX, .sun_path = "/dev/log"};
    int listener = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    assert(listener >= 0);
    assert(!bind(listener, (const struct sockaddr *)&address, sizeof(address)));
    assert(!listen(listener, 3));

    errno = EBUSY;
    openlog("native-log", LOG_PID | LOG_NDELAY, LOG_LOCAL2);
    assert(errno == EBUSY);
    int peer = accept_logger(listener);
    int previous = setlogmask(LOG_UPTO(LOG_WARNING));
    assert(previous == LOG_UPTO(LOG_DEBUG));
    syslog(LOG_INFO, "masked message");
    struct pollfd pending = {.fd = peer, .events = POLLIN};
    assert(poll(&pending, 1, 0) == 0);

    errno = ENOENT;
    syslog(LOG_WARNING, "open %s: %m (%%m)", "repo");
    assert(errno == ENOENT);
    char expected[128];
    int length = snprintf(expected, sizeof(expected), "native-log[%ld]: open repo: %s (%%m)",
                          (long)getpid(), strerror(ENOENT));
    assert(length > 0 && length < (int)sizeof(expected));
    expect_record(peer, LOG_LOCAL2 | LOG_WARNING, expected);

    assert(!close(peer));
    send_variadic(LOG_DAEMON | LOG_ERR, "ready%cagain", 0);
    peer = accept_logger(listener);
    expect_record(peer, LOG_DAEMON | LOG_ERR, "ready?again");
    assert(setlogmask(0) == LOG_UPTO(LOG_WARNING));
    assert(setlogmask(previous) == LOG_UPTO(LOG_WARNING));
    closelog();
    assert(!close(peer) && !close(listener));

    int output[2];
    assert(!pipe(output));
    int stderr_fd = dup(STDERR_FILENO);
    assert(stderr_fd >= 0 && dup2(output[1], STDERR_FILENO) == STDERR_FILENO);
    assert(!close(output[1]));
    openlog("fallback", LOG_PERROR, LOG_USER);
    syslog(LOG_NOTICE, "stderr %d", 25);
    closelog();
    assert(dup2(stderr_fd, STDERR_FILENO) == STDERR_FILENO && !close(stderr_fd));
    char line[64] = "";
    assert(read(output[0], line, sizeof(line) - 1) == sizeof("fallback: stderr 25\n") - 1);
    assert(!strcmp(line, "fallback: stderr 25\n") && !close(output[0]));
    puts("NATIVE_SYSLOG_PASS");
    return 0;
}
