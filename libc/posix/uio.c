/* SPDX-License-Identifier: BSD-3-Clause */
#include <sys/uio.h>
#include <sys/select.h>
#include <sys/time.h>
#include <poll.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "lume/abi.h"
#include "fd.h"
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
#include "native_posix.h"
#include "lume/file_io.h"
#include <fcntl.h>
#include <stdio.h>

static ssize_t native_vector_io(int fd, const struct iovec *vectors, int count, int writing,
                                int *handled)
{
    *handled = 0;
    /* Buffered stdin and terminal reads retain the stdio and termios path. */
    if (!writing && !fd)
        return 0;
    struct lume_io_vector local[8], *entries = local;
    if (count > 8)
    {
        entries = malloc((size_t)count * sizeof(*entries));
        if (!entries)
        {
            *handled = 1;
            return -1;
        }
    }
    for (int i = 0; i < count; ++i)
        entries[i] = (struct lume_io_vector){(uintptr_t)vectors[i].iov_base, vectors[i].iov_len};
    struct lume_file_io_request request = {.version = LUME_FILE_IO_VERSION,
                                           .bytes = sizeof(request),
                                           .fd = fd,
                                           .buffer = (uintptr_t)entries,
                                           .count = (unsigned)count};
    if (writing && (fd == 1 || fd == 2) && fflush(stdout) < 0 && fd == 1)
    {
        if (entries != local)
            free(entries);
        *handled = 1;
        return -1;
    }
    int64_t result;
    for (;;)
    {
        result = native_call(LUME_SYS_FILE_IO, writing ? LUME_FILE_IO_WRITEV : LUME_FILE_IO_READV,
                             (uintptr_t)&request, sizeof(request));
        if (result != -LUME_EAGAIN)
            break;
        int flags = fcntl(fd, F_GETFL);
        if (flags < 0 || flags & O_NONBLOCK)
            break;
        struct pollfd wait = {.fd = fd, .events = writing ? POLLOUT : POLLIN};
        int ready = poll(&wait, 1, -1);
        if (ready < 0)
        {
            if (entries != local)
                free(entries);
            *handled = 1;
            return -1;
        }
    }
    if (entries != local)
        free(entries);
    if (result == -LUME_ENOSYS || result == -LUME_EOPNOTSUPP || result == -LUME_EINVAL)
        return 0;
    *handled = 1;
    return native_result(result);
}

#define LUME_VECTOR_NATIVE 1
#endif

#include "vector_io.h"

ssize_t readv(int fd, const struct iovec *vectors, int count)
{
    return vector_io(fd, vectors, count, 0);
}

ssize_t writev(int fd, const struct iovec *vectors, int count)
{
    return vector_io(fd, vectors, count, 1);
}

int select(int maximum, fd_set *reads, fd_set *writes, fd_set *errors, struct timeval *timeout)
{
    if (maximum < 0 || maximum > FD_SETSIZE ||
        (timeout && (timeout->tv_sec < 0 || timeout->tv_usec < 0 || timeout->tv_usec >= 1000000)))
    {
        errno = EINVAL;
        return -1;
    }
    struct pollfd p[LUME_FD_LIMIT];
    unsigned count = 0;
    for (int i = 0; i < maximum; ++i)
        if ((reads && FD_ISSET(i, reads)) || (writes && FD_ISSET(i, writes)) ||
            (errors && FD_ISSET(i, errors)))
        {
            if ((unsigned)i >= LUME_FD_LIMIT)
            {
                errno = EBADF;
                return -1;
            }
            p[count++] = (struct pollfd){.fd = i,
                                         .events = (reads && FD_ISSET(i, reads) ? POLLIN : 0) |
                                                   (writes && FD_ISSET(i, writes) ? POLLOUT : 0) |
                                                   (errors && FD_ISSET(i, errors) ? POLLPRI : 0)};
        }
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
    /* Only the words addressed by maximum belong to this call. */
    size_t set_bytes = ((unsigned)maximum + sizeof(unsigned long) * 8 - 1) /
                       (sizeof(unsigned long) * 8) * sizeof(unsigned long);
    if (reads)
        memset(reads, 0, set_bytes);
    if (writes)
        memset(writes, 0, set_bytes);
    if (errors)
        memset(errors, 0, set_bytes);
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
        if (errors && (p[i].revents & POLLPRI))
        {
            FD_SET(p[i].fd, errors);
            ++total;
        }
    }
    return total;
}
