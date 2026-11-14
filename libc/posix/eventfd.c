/* SPDX-License-Identifier: BSD-3-Clause */
#include <sys/eventfd.h>
#include <errno.h>
#include <unistd.h>
#include "lume/abi.h"
#include "fd.h"
#include "lume/syscall_native.h"
#include "errno_map.h"

int eventfd(unsigned initial, int flags)
{
    if (flags & ~(EFD_CLOEXEC | EFD_NONBLOCK | EFD_SEMAPHORE))
    {
        errno = EINVAL;
        return -1;
    }
    struct lume_fd_request request = {.version = LUME_FD_VERSION,
                                      .bytes = sizeof(request),
                                      .count = initial,
                                      .flags =
                                          (flags & EFD_CLOEXEC ? LUME_FD_CLOEXEC : 0) |
                                          (flags & EFD_NONBLOCK ? LUME_FD_NONBLOCK : 0) |
                                          (flags & EFD_SEMAPHORE ? LUME_EVENTFD_SEMAPHORE : 0)};
    int64_t result =
        lume_native_call(LUME_SYS_FD, LUME_FD_EVENTFD, (uintptr_t)&request, sizeof(request));
    if (result < 0)
    {
        errno = lume_errno_from_syscall((unsigned)-result);
        return -1;
    }
    return request.fd;
}

int eventfd_read(int fd, eventfd_t *value)
{
    return read(fd, value, sizeof(*value)) == sizeof(*value) ? 0 : -1;
}

int eventfd_write(int fd, eventfd_t value)
{
    return write(fd, &value, sizeof(value)) == sizeof(value) ? 0 : -1;
}
