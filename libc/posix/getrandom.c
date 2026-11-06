/* SPDX-License-Identifier: BSD-3-Clause */
#include <sys/random.h>
#include <errno.h>
#include <stdint.h>
#include "lume/abi.h"
#include "errno_map.h"
#ifndef LUME_USER_HOST_TEST
#include "lume/syscall_native.h"
#else
extern int64_t lume_abi_host_call(unsigned, uint64_t, uint64_t, uint64_t);
#define lume_native_call lume_abi_host_call
#endif

ssize_t getrandom(void *buffer, size_t bytes, unsigned flags)
{
    if (flags & ~(GRND_NONBLOCK | GRND_RANDOM))
    {
        errno = EINVAL;
        return -1;
    }
    if (flags)
    {
        errno = ENOTSUP;
        return -1;
    }
    if (!bytes)
        return 0;
    if (!buffer)
    {
        errno = EFAULT;
        return -1;
    }
    size_t count = bytes > 256 ? 256 : bytes;
    int saved = errno;
    int64_t result = lume_native_call(LUME_SYS_RANDOM, (uintptr_t)buffer, count, 0);
    if (result < 0)
    {
        errno = lume_errno_from_syscall((unsigned)-result);
        return -1;
    }
    if (!result || (uint64_t)result > count)
    {
        errno = EIO;
        return -1;
    }
    errno = saved;
    return (ssize_t)result;
}
