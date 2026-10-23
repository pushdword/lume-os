#include "api.h"
#include "fd_client.h"
#include "errno_map.h"
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <stdarg.h>
#include <errno.h>

static int result(int value)
{
    if (value >= 0)
        return value;
    errno = lume_errno_from_syscall((unsigned)-value);
    return -1;
}

int pipe(int descriptors[2])
{
    if (!descriptors)
    {
        errno = EFAULT;
        return -1;
    }
    return result(app_fd_pair(descriptors, 0, 0));
}

int dup(int fd)
{
    return result(app_fd_dup(fd, 0, 0, 0));
}

int dup2(int fd, int to)
{
    return result(app_fd_dup(fd, to, 1, 0));
}

int fcntl(int fd, int operation, ...)
{
    int value = 0;
    if (operation == F_DUPFD || operation == F_SETFD || operation == F_SETFL
#ifdef F_DUPFD_CLOEXEC
        || operation == F_DUPFD_CLOEXEC
#endif
    )
    {
        va_list arguments;
        va_start(arguments, operation);
        value = va_arg(arguments, int);
        va_end(arguments);
    }
    switch (operation)
    {
    case F_DUPFD:
        if (value < 0)
        {
            errno = EINVAL;
            return -1;
        }
        return result(app_fd_dup(fd, value, 0, 0));
#ifdef F_DUPFD_CLOEXEC
    case F_DUPFD_CLOEXEC:
        if (value < 0)
        {
            errno = EINVAL;
            return -1;
        }
        return result(app_fd_dup(fd, value, 0, LUME_FD_CLOEXEC));
#endif
    case F_GETFD:
    {
        int flags = result(app_fd_flags(fd, LUME_FD_GETFD, 0));
        return flags < 0 ? -1 : (flags & LUME_FD_CLOEXEC) ? FD_CLOEXEC : 0;
    }
    case F_SETFD:
        if (value & ~FD_CLOEXEC)
        {
            errno = EINVAL;
            return -1;
        }
        return result(app_fd_flags(fd, LUME_FD_SETFD, value & FD_CLOEXEC ? LUME_FD_CLOEXEC : 0));
    case F_GETFL:
    {
        int mode = result(app_fd_flags(fd, LUME_FD_GETMODE, 0));
        if (mode < 0)
            return -1;
        int flags = result(app_fd_flags(fd, LUME_FD_GETFL, 0));
        return flags < 0 ? -1
                         : (mode & 3) | (mode & 0x400 ? O_APPEND : 0) |
                               (flags & LUME_FD_NONBLOCK ? O_NONBLOCK : 0);
    }
    case F_SETFL:
        if (value & ~(O_ACCMODE | O_APPEND | O_NONBLOCK))
        {
            errno = ENOTSUP;
            return -1;
        }
        if (result(app_fd_flags(fd, LUME_FD_SETMODE, value & O_APPEND ? 0x400 : 0)) < 0)
            return -1;
        return result(app_fd_flags(fd, LUME_FD_SETFL, value & O_NONBLOCK ? LUME_FD_NONBLOCK : 0));
    default:
        errno = ENOTSUP;
        return -1;
    }
}

int poll(struct pollfd *fds, nfds_t count, int timeout)
{
    if (count > LUME_FD_LIMIT || timeout < -1 || timeout > 60000 || (!fds && count))
    {
        errno = EINVAL;
        return -1;
    }
    struct lume_pollfd records[LUME_FD_LIMIT];
    for (unsigned i = 0; i < count; ++i)
    {
        records[i] = (struct lume_pollfd){.fd = fds[i].fd,
                                          .events = (fds[i].events & POLLIN ? LUME_POLLIN : 0) |
                                                    (fds[i].events & POLLOUT ? LUME_POLLOUT : 0)};
        fds[i].revents = 0;
    }
    int ready =
        result(app_poll(records, (unsigned)count, timeout < 0 ? UINT32_MAX : (uint32_t)timeout));
    if (ready < 0)
        return -1;
    for (unsigned i = 0; i < count; ++i)
        fds[i].revents = (records[i].revents & LUME_POLLIN ? POLLIN : 0) |
                         (records[i].revents & LUME_POLLOUT ? POLLOUT : 0) |
                         (records[i].revents & LUME_POLLERR ? POLLERR : 0) |
                         (records[i].revents & LUME_POLLHUP ? POLLHUP : 0) |
                         (records[i].revents & LUME_POLLNVAL ? POLLNVAL : 0);
    return ready;
}
