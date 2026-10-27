/* SPDX-License-Identifier: BSD-3-Clause */
#include "api.h"
#include "errno_map.h"
#include <errno.h>
#include <stdint.h>
#include <stddef.h>

void *sbrk(ptrdiff_t increment)
{
    int value = lume_syscall(35, 0, 0, 0);
    if (value < 0)
    {
        errno = lume_errno_from_syscall((unsigned)-value);
        return (void *)-1;
    }
    uintptr_t old = (uint32_t)value, requested;
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
    value = lume_syscall(35, (uint32_t)requested, 0, 0);
    if (value < 0)
    {
        errno = lume_errno_from_syscall((unsigned)-value);
        return (void *)-1;
    }
    if ((uint32_t)value != requested)
    {
        errno = ENOMEM;
        return (void *)-1;
    }
    return (void *)old;
}
