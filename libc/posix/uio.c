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

static ssize_t vector_io(int fd, const struct iovec *vectors, int count, int writing)
{
    if (count < 0 || count > 1024)
    {
        errno = EINVAL;
        return -1;
    }
    size_t bytes = 0;
    for (int i = 0; i < count; ++i)
    {
        if (vectors[i].iov_len > SIZE_MAX / 2 - bytes)
        {
            errno = EINVAL;
            return -1;
        }
        bytes += vectors[i].iov_len;
    }
    if (!bytes)
        return 0;
    if (bytes > 65536)
        bytes = 65536;
    char *buffer = malloc(bytes);
    if (!buffer)
        return -1;
    size_t offset = 0;
    if (writing)
        for (int i = 0; i < count && offset < bytes; ++i)
        {
            size_t length = vectors[i].iov_len;
            if (length > bytes - offset)
                length = bytes - offset;
            memcpy(buffer + offset, vectors[i].iov_base, length);
            offset += length;
        }
    ssize_t result = writing ? write(fd, buffer, bytes) : read(fd, buffer, bytes);
    if (!writing && result > 0)
    {
        offset = 0;
        for (int i = 0; i < count && offset < (size_t)result; ++i)
        {
            size_t length = vectors[i].iov_len;
            if (length > (size_t)result - offset)
                length = (size_t)result - offset;
            memcpy(vectors[i].iov_base, buffer + offset, length);
            offset += length;
        }
    }
    free(buffer);
    return result;
}

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
    if (maximum < 0 || maximum > LUME_FD_LIMIT ||
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
            p[count++] = (struct pollfd){.fd = i,
                                         .events = (reads && FD_ISSET(i, reads) ? POLLIN : 0) |
                                                   (writes && FD_ISSET(i, writes) ? POLLOUT : 0) |
                                                   (errors && FD_ISSET(i, errors) ? POLLPRI : 0)};
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
        if (errors && (p[i].revents & POLLPRI))
        {
            FD_SET(p[i].fd, errors);
            ++total;
        }
    }
    return total;
}
