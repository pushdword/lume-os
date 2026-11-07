#include "api.h"
#include "fd_client.h"
#include "errno_map.h"
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <stdarg.h>
#include <errno.h>
#if defined(__aarch64__) || defined(__x86_64__) || defined(__i386__)
#include "lume/memory.h"
extern int64_t lume_memory_call(unsigned, struct lume_vm_request *);
#endif

extern int lume_posix_ofd_lock(int, unsigned, struct flock *);
extern int lume_posix_record_lock(int, unsigned, struct flock *);

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

int pipe2(int descriptors[2], int flags)
{
    if (flags & ~(O_CLOEXEC | O_NONBLOCK))
    {
        errno = EINVAL;
        return -1;
    }
    if (!descriptors)
    {
        errno = EFAULT;
        return -1;
    }
    unsigned native_flags =
        (flags & O_CLOEXEC ? LUME_FD_CLOEXEC : 0) | (flags & O_NONBLOCK ? LUME_FD_NONBLOCK : 0);
    return result(app_fd_pair(descriptors, 0, native_flags));
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
    if (operation == F_SETLKW)
    {
        errno = ENOTSUP;
        return -1;
    }
    if (operation == F_OFD_GETLK || operation == F_OFD_SETLK || operation == F_GETLK ||
        operation == F_SETLK)
    {
        va_list arguments;
        va_start(arguments, operation);
        struct flock *lock = va_arg(arguments, struct flock *);
        va_end(arguments);
        return operation == F_GETLK || operation == F_SETLK
                   ? lume_posix_record_lock(fd, (unsigned)operation, lock)
                   : lume_posix_ofd_lock(fd, (unsigned)operation, lock);
    }
    int value = 0;
    if (operation == F_DUPFD || operation == F_SETFD || operation == F_SETFL
#ifdef F_ADD_SEALS
        || operation == F_ADD_SEALS
#endif
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
#if (defined(__aarch64__) || defined(__x86_64__) || defined(__i386__)) && defined(F_ADD_SEALS)
    case F_ADD_SEALS:
    case F_GET_SEALS:
    {
        struct lume_vm_request request = {.fd = fd, .flags = (unsigned)value};
        int64_t sealed = lume_memory_call(
            operation == F_ADD_SEALS ? LUME_VM_ADD_SEALS : LUME_VM_GET_SEALS, &request);
        if (sealed < 0)
        {
            errno = (int)-sealed;
            return -1;
        }
        return (int)sealed;
    }
#endif
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
    if (count > LUME_FD_LIMIT || (!fds && count))
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
    int ready;
    do
    {
        unsigned step = timeout < 0 ? UINT32_MAX : timeout > 60000 ? 60000u : (unsigned)timeout;
        ready = result(app_poll(records, (unsigned)count, step));
        if (ready || timeout <= 60000)
            break;
        timeout -= 60000;
    } while (1);
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
