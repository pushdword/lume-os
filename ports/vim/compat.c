#include "../standalone/native.h"
#include <lume/credential.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <poll.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

extern int vim_main(int, char **);
extern void lume_stdio_raw(int);
extern int __real_open(const char *, int, ...);

int __wrap_open(const char *path, int flags, ...)
{
    mode_t mode = 0666;
    if (flags & O_CREAT)
    {
        va_list arguments;
        va_start(arguments, flags);
        mode = (mode_t)va_arg(arguments, int);
        va_end(arguments);
    }
    /* The VFS has no symbolic links. Its path lookup already satisfies O_NOFOLLOW. */
    return __real_open(path, flags & ~O_NOFOLLOW, mode);
}

int main(int argc, char **argv)
{
    lume_stdio_raw(1);
    return vim_main(argc, argv);
}

int tcgetattr(int fd, struct termios *attributes)
{
    (void)fd;
    (void)attributes;
    errno = ENOTTY;
    return -1;
}

int tcsetattr(int fd, int action, const struct termios *attributes)
{
    (void)fd;
    (void)action;
    (void)attributes;
    errno = ENOTTY;
    return -1;
}

int ioctl(int fd, unsigned long operation, ...)
{
    if (operation != TIOCGWINSZ || !isatty(fd))
    {
        errno = ENOTTY;
        return -1;
    }
    va_list arguments;
    va_start(arguments, operation);
    struct winsize *size = va_arg(arguments, struct winsize *);
    va_end(arguments);
    if (!size)
    {
        errno = EFAULT;
        return -1;
    }
    int geometry = native_result(LUME_SYS_TERMINAL, 0, 0, 0);
    if (geometry < 0)
    {
        return -1;
    }
    *size =
        (struct winsize){.ws_row = (unsigned)geometry >> 16, .ws_col = (unsigned)geometry & 65535};
    return 0;
}

void term_set_winsize(int height, int width)
{
    if (height > 0 && width > 0)
        printf("\033[8;%d;%dt", height, width);
}

int select(int maximum, fd_set *reads, fd_set *writes, fd_set *errors, struct timeval *timeout)
{
    if (maximum < 0 || maximum > LUME_FD_MAX ||
        (timeout && (timeout->tv_sec < 0 || timeout->tv_usec < 0 || timeout->tv_usec >= 1000000)))
    {
        errno = EINVAL;
        return -1;
    }
    struct pollfd descriptors[LUME_FD_MAX];
    unsigned count = 0;
    for (int fd = 0; fd < maximum; ++fd)
        if ((reads && FD_ISSET(fd, reads)) || (writes && FD_ISSET(fd, writes)) ||
            (errors && FD_ISSET(fd, errors)))
            descriptors[count++] =
                (struct pollfd){.fd = fd,
                                .events = (reads && FD_ISSET(fd, reads) ? POLLIN : 0) |
                                          (writes && FD_ISSET(fd, writes) ? POLLOUT : 0)};
    int64_t remaining =
        timeout ? (int64_t)timeout->tv_sec * 1000 + (timeout->tv_usec + 999) / 1000 : -1;
    int ready;
    do
    {
        int wait = remaining < 0 ? -1 : remaining > 60000 ? 60000 : (int)remaining;
        ready = poll(descriptors, count, wait);
        if (ready || remaining <= 60000)
            break;
        remaining -= 60000;
    } while (1);
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
        struct pollfd *descriptor = descriptors + i;
        if (descriptor->revents & POLLNVAL)
        {
            errno = EBADF;
            return -1;
        }
        if (reads && (descriptor->events & POLLIN) &&
            (descriptor->revents & (POLLIN | POLLHUP | POLLERR)))
        {
            FD_SET(descriptor->fd, reads);
            ++total;
        }
        if (writes && (descriptor->events & POLLOUT) && (descriptor->revents & (POLLOUT | POLLERR)))
        {
            FD_SET(descriptor->fd, writes);
            ++total;
        }
        if (errors && (descriptor->revents & POLLERR))
        {
            FD_SET(descriptor->fd, errors);
            ++total;
        }
    }
    return total;
}

struct passwd *getpwuid(uid_t uid)
{
    static struct passwd account = {.pw_name = "root",
                                    .pw_passwd = "*",
                                    .pw_uid = 0,
                                    .pw_gid = 0,
                                    .pw_dir = "/data/home",
                                    .pw_shell = "/bin/sh"};
    if (uid)
    {
        errno = ENOENT;
        return NULL;
    }
    return &account;
}

struct group *getgrgid(gid_t gid)
{
    static char *members[] = {"root", NULL};
    static struct group group = {
        .gr_name = "root", .gr_passwd = "*", .gr_gid = 0, .gr_mem = members};
    if (gid)
    {
        errno = ENOENT;
        return NULL;
    }
    return &group;
}

mode_t umask(mode_t mask)
{
    struct lume_credential_request request = {
        .version = LUME_CREDENTIAL_VERSION, .bytes = sizeof(request), .uid = mask};
    return native_result(LUME_SYS_CREDENTIAL, LUME_CREDENTIAL_UMASK, (uintptr_t)&request,
                         sizeof(request));
}

int gethostname(char *buffer, size_t bytes)
{
    if (bytes < sizeof("lume"))
    {
        errno = ENAMETOOLONG;
        return -1;
    }
    memcpy(buffer, "lume", sizeof("lume"));
    return 0;
}
