/* SPDX-License-Identifier: BSD-3-Clause */
#include "errno_map.h"
#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#if defined(__aarch64__) || defined(__x86_64__) || defined(LUME_ARCH_I686)
#include "lume/abi.h"
#include "lume/break.h"
#include "lume/syscall_native.h"
#else
#include "api.h"
#endif

static int64_t request_break(uintptr_t address)
{
#if defined(__aarch64__) || defined(__x86_64__) || defined(LUME_ARCH_I686)
#if defined(LUME_ARCH_I686)
    uintptr_t code = (uintptr_t)&sbrk;
    uint64_t flags =
        code >= UINT32_C(0x70000000) && code < UINT32_C(0x90000000) ? LUME_BRK_EXTENDED : 0;
#else
    uint64_t flags = (uintptr_t)&sbrk > UINT32_MAX ? LUME_BRK_EXTENDED : 0;
#endif
    return lume_native_call(LUME_SYS_BRK, address, flags, 0);
#else
    return lume_syscall(35, (uint32_t)address, 0, 0);
#endif
}

void *sbrk(ptrdiff_t increment)
{
    int64_t value = request_break(0);
    if (value < 0)
    {
        errno = lume_errno_from_syscall((unsigned)-value);
        return (void *)-1;
    }
    uintptr_t old = (uintptr_t)value, requested;
    if (!increment)
        return (void *)old;
    if (increment >= 0)
    {
        if ((uintptr_t)increment > UINTPTR_MAX - old)
        {
            errno = ENOMEM;
            return (void *)-1;
        }
        requested = old + (uintptr_t)increment;
    }
    else
    {
        uintptr_t magnitude = (uintptr_t)(-(increment + 1)) + 1;
        if (magnitude > old)
        {
            errno = ENOMEM;
            return (void *)-1;
        }
        requested = old - magnitude;
    }
    value = request_break(requested);
    if (value < 0)
    {
        errno = lume_errno_from_syscall((unsigned)-value);
        return (void *)-1;
    }
    if ((uint64_t)value != requested)
    {
        errno = ENOMEM;
        return (void *)-1;
    }
    return (void *)old;
}
