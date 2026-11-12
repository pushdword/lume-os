/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_XSHMFENCE_FUTEX_H
#define LUME_XSHMFENCE_FUTEX_H
#include "lume/abi.h"
#include "lume/thread.h"
#include "lume/syscall_native.h"
#include "errno_map.h"
#include <errno.h>
#include <stdint.h>

static inline int lume_fence_word_call(unsigned action, int32_t *address, uint32_t value)
{
    struct lume_thread_request request = {
        .address = (uintptr_t)address, .value = value, .timeout_ms = UINT64_MAX};
    int64_t result =
        lume_native_call(LUME_SYS_THREAD, action, (uintptr_t)&request, sizeof(request));
    if (result < 0)
    {
        errno = lume_errno_from_syscall((unsigned)-result);
        return -1;
    }
    return (int)result;
}

static inline int futex_wake(int32_t *address)
{
    return lume_fence_word_call(LUME_THREAD_WAKE_SHARED, address, UINT32_MAX);
}

static inline int futex_wait(int32_t *address, int32_t value)
{
    return lume_fence_word_call(LUME_THREAD_WAIT_SHARED, address, (uint32_t)value);
}
#endif
