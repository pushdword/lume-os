#include "native.h"
#include "lume/credential.h"
#include "lume/file_mode.h"
#include "lume/signal.h"
#include "fd.h"
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <termios.h>
#include <time.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

int getentropy(void *data, size_t bytes)
{
    if (bytes > 256)
    {
        errno = EIO;
        return -1;
    }
    int result = native_result(native_call(LUME_SYS_RANDOM, (uintptr_t)data, bytes, 0));
    return result < 0 ? -1 : 0;
}

int lume_ssh_flush(FILE *stream)
{
    if (stream)
        return (fflush)(stream);
    int result = (fflush)(stdout);
    return (fflush)(stderr) || result ? EOF : 0;
}

ssize_t getdelim(char **line, size_t *capacity, int delimiter, FILE *stream)
{
    if (!line || !capacity || !stream)
    {
        errno = EINVAL;
        return -1;
    }
    if (!*line)
        *capacity = 0;
    size_t used = 0;
    for (;;)
    {
        int c = fgetc(stream);
        if (c == EOF)
        {
            if (!used || ferror(stream))
                return -1;
            break;
        }
        if (used + 1 >= *capacity)
        {
            if (*capacity > (size_t)SSIZE_MAX / 2)
            {
                errno = EOVERFLOW;
                return -1;
            }
            size_t count = *capacity ? *capacity * 2 : 128;
            char *buffer = realloc(*line, count);
            if (!buffer)
            {
                errno = ENOMEM;
                return -1;
            }
            *line = buffer;
            *capacity = count;
        }
        (*line)[used++] = c;
        if ((unsigned char)c == (unsigned char)delimiter)
            break;
    }
    (*line)[used] = 0;
    return (ssize_t)used;
}

ssize_t getline(char **line, size_t *capacity, FILE *stream)
{
    return getdelim(line, capacity, '\n', stream);
}

static struct passwd accounts[] = {{.pw_name = "root",
                                    .pw_passwd = "*",
                                    .pw_uid = 0,
                                    .pw_gid = 0,
                                    .pw_comment = "Administrator",
                                    .pw_gecos = "Administrator",
                                    .pw_dir = "/data/root",
                                    .pw_shell = "/bin/sh"},
                                   {.pw_name = "sshd",
                                    .pw_passwd = "*",
                                    .pw_uid = 65534,
                                    .pw_gid = 65534,
                                    .pw_comment = "SSH authentication",
                                    .pw_gecos = "SSH authentication",
                                    .pw_dir = "/data/var/empty-sshd",
                                    .pw_shell = "/bin/false"}};
static char *empty_members[] = {NULL};
static struct group groups[] = {
    {.gr_name = "root", .gr_passwd = "*", .gr_gid = 0, .gr_mem = empty_members},
    {.gr_name = "sshd", .gr_passwd = "*", .gr_gid = 65534, .gr_mem = empty_members}};
static unsigned next_account, next_group;

struct passwd *getpwnam(const char *name)
{
    for (unsigned i = 0; i < 2; ++i)
        if (!strcmp(name, accounts[i].pw_name))
            return &accounts[i];
    return NULL;
}

struct passwd *getpwuid(uid_t uid)
{
    for (unsigned i = 0; i < 2; ++i)
        if (accounts[i].pw_uid == uid)
            return &accounts[i];
    return NULL;
}

void setpwent(void)
{
    next_account = 0;
}

void endpwent(void)
{
    next_account = 0;
}

struct passwd *getpwent(void)
{
    return next_account < 2 ? &accounts[next_account++] : NULL;
}

struct group *getgrnam(const char *name)
{
    for (unsigned i = 0; i < 2; ++i)
        if (!strcmp(name, groups[i].gr_name))
            return &groups[i];
    return NULL;
}

struct group *getgrgid(gid_t gid)
{
    for (unsigned i = 0; i < 2; ++i)
        if (groups[i].gr_gid == gid)
            return &groups[i];
    return NULL;
}

void setgrent(void)
{
    next_group = 0;
}

void endgrent(void)
{
    next_group = 0;
}

struct group *getgrent(void)
{
    return next_group < 2 ? &groups[next_group++] : NULL;
}

int initgroups(const char *name, gid_t gid)
{
    if (!getpwnam(name))
    {
        errno = ENOENT;
        return -1;
    }
    return setgroups(1, &gid);
}

char *crypt(const char *key, const char *salt)
{
    (void)key;
    (void)salt;
    errno = ENOSYS;
    return NULL;
}

int innetgr(const char *group, const char *host, const char *user, const char *domain)
{
    (void)group;
    (void)host;
    (void)user;
    (void)domain;
    return 0;
}

static int change_mode(const char *path, int fd, unsigned mode, uid_t uid, gid_t gid)
{
    struct lume_file_mode_request r = {.version = LUME_FILE_MODE_VERSION,
                                       .bytes = sizeof(r),
                                       .path = (uintptr_t)path,
                                       .fd = fd,
                                       .mode = mode,
                                       .uid = uid,
                                       .gid = gid};
    return native_result(native_call(LUME_SYS_FILE_MODE,
                                     path ? LUME_FILE_MODE_PATH : LUME_FILE_MODE_FD, (uintptr_t)&r,
                                     sizeof(r)));
}

int chmod(const char *path, mode_t mode)
{
    return change_mode(path, -1, mode, 0, 0);
}

int fchmod(int fd, mode_t mode)
{
    return change_mode(NULL, fd, mode, 0, 0);
}

int chown(const char *path, uid_t uid, gid_t gid)
{
    return change_mode(path, -1, UINT32_MAX, uid == (uid_t)-1 ? 0 : uid,
                       gid == (gid_t)-1 ? 0 : gid);
}

int fchown(int fd, uid_t uid, gid_t gid)
{
    return change_mode(NULL, fd, UINT32_MAX, uid == (uid_t)-1 ? 0 : uid,
                       gid == (gid_t)-1 ? 0 : gid);
}

int lchown(const char *path, uid_t uid, gid_t gid)
{
    return chown(path, uid, gid);
}

int fchmodat(int directory, const char *path, mode_t mode, int flags)
{
    if (flags || (directory != AT_FDCWD && *path != '/'))
        return unsupported();
    return chmod(path, mode);
}

int fchownat(int directory, const char *path, uid_t uid, gid_t gid, int flags)
{
    if (flags || (directory != AT_FDCWD && *path != '/'))
        return unsupported();
    return chown(path, uid, gid);
}

mode_t umask(mode_t mask)
{
    struct lume_credential_request r = {
        .version = LUME_CREDENTIAL_VERSION, .bytes = sizeof(r), .uid = mask};
    return native_result(
        native_call(LUME_SYS_CREDENTIAL, LUME_CREDENTIAL_UMASK, (uintptr_t)&r, sizeof(r)));
}

int link(const char *old, const char *path)
{
    (void)old;
    (void)path;
    return unsupported();
}

int symlink(const char *target, const char *path)
{
    (void)target;
    (void)path;
    return unsupported();
}

ssize_t readlink(const char *path, char *buffer, size_t bytes)
{
    (void)path;
    (void)buffer;
    (void)bytes;
    errno = EINVAL;
    return -1;
}

int utimes(const char *path, const struct timeval times[2])
{
    (void)path;
    (void)times;
    return unsupported();
}

int futimes(int fd, const struct timeval times[2])
{
    (void)fd;
    (void)times;
    return unsupported();
}

int statvfs(const char *path, struct statvfs *out)
{
    (void)path;
    (void)out;
    return unsupported();
}

int fstatvfs(int fd, struct statvfs *out)
{
    (void)fd;
    (void)out;
    return unsupported();
}

char *realpath(const char *path, char *resolved)
{
    char input[LUME_PATH_MAX], result[LUME_PATH_MAX];
    if (!path || !*path)
    {
        errno = ENOENT;
        return NULL;
    }
    if (*path == '/')
    {
        if (strlen(path) >= sizeof(input))
        {
            errno = ENAMETOOLONG;
            return NULL;
        }
        strcpy(input, path);
    }
    else
    {
        char cwd[LUME_PATH_MAX];
        if (!getcwd(cwd, sizeof(cwd)))
            return NULL;
        if (snprintf(input, sizeof(input), "%s/%s", cwd, path) >= (int)sizeof(input))
        {
            errno = ENAMETOOLONG;
            return NULL;
        }
    }
    strcpy(result, "/");
    char *save, *part = strtok_r(input, "/", &save);
    while (part)
    {
        if (!strcmp(part, ".."))
        {
            char *slash = strrchr(result, '/');
            if (slash == result)
                result[1] = 0;
            else
                *slash = 0;
        }
        else if (strcmp(part, "."))
        {
            if (result[1])
                strcat(result, "/");
            strcat(result, part);
            struct stat metadata;
            if (stat(result, &metadata))
                return NULL;
            if (save && *save && !S_ISDIR(metadata.st_mode))
            {
                errno = ENOTDIR;
                return NULL;
            }
        }
        part = strtok_r(NULL, "/", &save);
    }
    if (!resolved && !(resolved = malloc(strlen(result) + 1)))
    {
        errno = ENOMEM;
        return NULL;
    }
    strcpy(resolved, result);
    return resolved;
}

int gethostname(char *name, size_t bytes)
{
    if (bytes < sizeof("lume"))
    {
        errno = ENAMETOOLONG;
        return -1;
    }
    memcpy(name, "lume", sizeof("lume"));
    return 0;
}

int uname(struct utsname *out)
{
    memset(out, 0, sizeof(*out));
    strcpy(out->sysname, "Lume");
    strcpy(out->nodename, "lume");
    strcpy(out->release, "1.0");
    strcpy(out->version, "AArch64 native ABI 1.0");
    strcpy(out->machine, "aarch64");
    return 0;
}

void openlog(const char *name, int flags, int facility)
{
    (void)name;
    (void)flags;
    (void)facility;
}

void closelog(void)
{
}

void syslog(int priority, const char *format, ...)
{
    (void)priority;
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    fputc('\n', stderr);
}

int nanosleep(const struct timespec *delay, struct timespec *remaining)
{
    if (!delay || delay->tv_sec < 0 || delay->tv_nsec < 0 || delay->tv_nsec >= 1000000000)
    {
        errno = EINVAL;
        return -1;
    }
    struct timespec before, after;
    clock_gettime(CLOCK_MONOTONIC, &before);
    uint64_t ms = (uint64_t)delay->tv_sec * 1000 + (delay->tv_nsec + 999999) / 1000000;
    while (ms)
    {
        unsigned step = ms > 60000 ? 60000 : (unsigned)ms;
        if (native_result(native_call(LUME_SYS_SLEEP, step, 0, 0)) < 0)
        {
            if (remaining)
            {
                clock_gettime(CLOCK_MONOTONIC, &after);
                int64_t ns =
                    (int64_t)delay->tv_sec * 1000000000 + delay->tv_nsec -
                    ((after.tv_sec - before.tv_sec) * 1000000000 + after.tv_nsec - before.tv_nsec);
                if (ns < 0)
                    ns = 0;
                *remaining = (struct timespec){ns / 1000000000, ns % 1000000000};
            }
            return -1;
        }
        ms -= step;
    }
    if (remaining)
        memset(remaining, 0, sizeof(*remaining));
    return 0;
}

int usleep(useconds_t usec)
{
    struct timespec delay = {usec / 1000000, (usec % 1000000) * 1000};
    return nanosleep(&delay, NULL);
}

int select(int maximum, fd_set *reads, fd_set *writes, fd_set *errors, struct timeval *timeout)
{
    if (maximum < 0 || maximum > LUME_FD_MAX ||
        (timeout && (timeout->tv_sec < 0 || timeout->tv_usec < 0 || timeout->tv_usec >= 1000000)))
    {
        errno = EINVAL;
        return -1;
    }
    struct pollfd p[LUME_FD_MAX];
    unsigned count = 0;
    for (int i = 0; i < maximum; ++i)
        if ((reads && FD_ISSET(i, reads)) || (writes && FD_ISSET(i, writes)) ||
            (errors && FD_ISSET(i, errors)))
            p[count++] = (struct pollfd){.fd = i,
                                         .events = (reads && FD_ISSET(i, reads) ? POLLIN : 0) |
                                                   (writes && FD_ISSET(i, writes) ? POLLOUT : 0)};
    int64_t ms = timeout ? (int64_t)timeout->tv_sec * 1000 + (timeout->tv_usec + 999) / 1000 : -1;
    int ready;
    do
    {
        ready = poll(p, count, ms < 0 ? -1 : ms > 60000 ? 60000 : (int)ms);
        if (ms > 60000)
            ms -= 60000;
        else
            break;
    } while (!ready);
    if (ready < 0)
        return -1;
    if (reads)
        FD_ZERO(reads);
    if (writes)
        FD_ZERO(writes);
    if (errors)
        FD_ZERO(errors);
    int total = 0;
    for (unsigned i = 0; i < count; ++i)
    {
        if (p[i].revents & POLLNVAL)
        {
            errno = EBADF;
            return -1;
        }
        if (reads && (p[i].events & POLLIN) && (p[i].revents & (POLLIN | POLLHUP | POLLERR)))
        {
            FD_SET(p[i].fd, reads);
            ++total;
        }
        if (writes && (p[i].events & POLLOUT) && (p[i].revents & (POLLOUT | POLLERR)))
        {
            FD_SET(p[i].fd, writes);
            ++total;
        }
        if (errors && (p[i].revents & POLLERR))
        {
            FD_SET(p[i].fd, errors);
            ++total;
        }
    }
    return total;
}

int ppoll(struct pollfd *fds, nfds_t count, const struct timespec *timeout, const sigset_t *mask)
{
    if (count > LUME_FD_LIMIT || (timeout && (timeout->tv_sec < 0 || timeout->tv_nsec < 0 ||
                                              timeout->tv_nsec >= 1000000000)))
    {
        errno = EINVAL;
        return -1;
    }
    struct lume_signal_request signal = {.version = LUME_SIGNAL_VERSION,
                                         .bytes = sizeof(signal),
                                         .operation = 0,
                                         .flags = mask != NULL,
                                         .mask = mask ? *mask : 0};
    if (native_result(
            native_call(LUME_SYS_SIGNAL, LUME_SIGNAL_MASK, (uintptr_t)&signal, sizeof(signal))) < 0)
        return -1;
    sigset_t old = signal.mask;
    uint64_t epoch = signal.delay_ms;
    struct lume_pollfd records[LUME_FD_LIMIT];
    for (unsigned i = 0; i < count; ++i)
    {
        records[i] = (struct lume_pollfd){.fd = fds[i].fd,
                                          .events = (fds[i].events & POLLIN ? LUME_POLLIN : 0) |
                                                    (fds[i].events & POLLOUT ? LUME_POLLOUT : 0)};
        fds[i].revents = 0;
    }
    uint64_t start = native_call(LUME_SYS_CLOCK, 0, 0, 0);
    uint64_t ms = timeout ? (uint64_t)timeout->tv_sec * 1000 + (timeout->tv_nsec + 999999) / 1000000
                          : UINT64_MAX;
    struct lume_fd_request request = {.version = LUME_FD_VERSION,
                                      .bytes = sizeof(request),
                                      .buffer = (uint32_t)(uintptr_t)records,
                                      .count = count};
    int result = 0;
    if (signal.interval_ms)
    {
        errno = EINTR;
        result = -1;
    }
    else
        for (;;)
        {
            result = native_result(
                native_call(LUME_SYS_FD, LUME_FD_POLL, (uintptr_t)&request, sizeof(request)));
            if (result || (uint64_t)native_call(LUME_SYS_CLOCK, 0, 0, 0) - start >= ms)
                break;
            if (native_result(native_call(LUME_SYS_SLEEP, 1, epoch, 1)) < 0)
            {
                result = -1;
                break;
            }
        }
    int saved = errno;
    if (mask && sigprocmask(SIG_SETMASK, &old, NULL) < 0)
        return -1;
    errno = saved;
    if (result >= 0)
        for (unsigned i = 0; i < count; ++i)
            fds[i].revents = (records[i].revents & LUME_POLLIN ? POLLIN : 0) |
                             (records[i].revents & LUME_POLLOUT ? POLLOUT : 0) |
                             (records[i].revents & LUME_POLLERR ? POLLERR : 0) |
                             (records[i].revents & LUME_POLLHUP ? POLLHUP : 0) |
                             (records[i].revents & LUME_POLLNVAL ? POLLNVAL : 0);
    return result;
}

static struct
{
    FILE *file;
    pid_t pid;
} process_pipes[LUME_FD_MAX];

FILE *popen(const char *command, const char *mode)
{
    if (!mode || ((*mode != 'r' && *mode != 'w') || mode[1]))
    {
        errno = EINVAL;
        return NULL;
    }
    unsigned slot;
    for (slot = 0; slot < LUME_FD_MAX && process_pipes[slot].file; ++slot)
    {
    }
    if (slot == LUME_FD_MAX)
    {
        errno = EMFILE;
        return NULL;
    }
    int p[2];
    if (pipe(p))
        return NULL;
    pid_t child = fork();
    if (!child)
    {
        unsigned source = *mode == 'r' ? 1 : 0, target = *mode == 'r' ? 1 : 0;
        close(p[source ^ 1]);
        if (dup2(p[source], target) < 0)
            _exit(126);
        if (p[source] != (int)target)
            close(p[source]);
        execl("/bin/sh", "sh", "-c", command, (char *)NULL);
        _exit(127);
    }
    int parent = *mode == 'r' ? 0 : 1;
    close(p[parent ^ 1]);
    if (child < 0)
    {
        close(p[parent]);
        return NULL;
    }
    FILE *file = fdopen(p[parent], mode);
    if (!file)
    {
        close(p[parent]);
        while (waitpid(child, NULL, 0) < 0 && errno == EINTR)
        {
        }
        return NULL;
    }
    process_pipes[slot].file = file;
    process_pipes[slot].pid = child;
    return file;
}

int pclose(FILE *file)
{
    for (unsigned i = 0; i < LUME_FD_MAX; ++i)
        if (process_pipes[i].file == file)
        {
            pid_t child = process_pipes[i].pid;
            process_pipes[i].file = NULL;
            int close_result = fclose(file), status, result;
            do
            {
                result = waitpid(child, &status, 0);
            } while (result < 0 && errno == EINTR);
            return result < 0 || close_result ? -1 : status;
        }
    errno = EINVAL;
    return -1;
}

int openpty(int *master, int *slave, char *name, const struct termios *term,
            const struct winsize *size)
{
    (void)master;
    (void)slave;
    (void)name;
    (void)term;
    (void)size;
    return unsupported();
}

int ioctl(int fd, unsigned long request, ...)
{
    (void)fd;
    (void)request;
    errno = ENOTTY;
    return -1;
}

char *ttyname(int fd)
{
    (void)fd;
    errno = ENOTTY;
    return NULL;
}

int tcgetattr(int fd, struct termios *t)
{
    (void)fd;
    (void)t;
    errno = ENOTTY;
    return -1;
}

int tcsetattr(int fd, int when, const struct termios *t)
{
    (void)fd;
    (void)when;
    (void)t;
    errno = ENOTTY;
    return -1;
}

int tcsendbreak(int fd, int duration)
{
    (void)fd;
    (void)duration;
    errno = ENOTTY;
    return -1;
}

int tcdrain(int fd)
{
    (void)fd;
    errno = ENOTTY;
    return -1;
}

int cfsetispeed(struct termios *t, speed_t speed)
{
    t->c_ispeed = speed;
    return 0;
}

int cfsetospeed(struct termios *t, speed_t speed)
{
    t->c_ospeed = speed;
    return 0;
}
