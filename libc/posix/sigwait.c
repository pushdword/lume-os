/* SPDX-License-Identifier: BSD-3-Clause */
#include <signal.h>
#include <errno.h>
#include <stdint.h>
#include "lume/abi.h"
#include "lume/signal.h"
#include "errno_map.h"
#ifndef LUME_USER_HOST_TEST
#include "lume/syscall_native.h"
#else
extern int64_t lume_abi_host_call(unsigned, uint64_t, uint64_t, uint64_t);
#define lume_native_call lume_abi_host_call
#endif

int sigwait(const sigset_t *set, int *number)
{
    int saved = errno, error = 0;
    if (!set || !number)
    {
        error = EFAULT;
        goto done;
    }
    sigset_t selected = *set;
    if (!selected || (selected & ((UINT32_C(1) << SIGKILL) | (UINT32_C(1) << SIGSTOP) | 1u)))
    {
        error = EINVAL;
        goto done;
    }
    for (;;)
    {
        struct lume_signal_request request = {
            .version = LUME_SIGNAL_VERSION, .bytes = sizeof(request), .mask = selected};
        int64_t result = lume_native_call(LUME_SYS_SIGNAL, LUME_SIGNAL_TRY_WAIT,
                                          (uintptr_t)&request, sizeof(request));
        if (!result)
        {
            if (!request.number || request.number >= NSIG ||
                !(selected & (UINT32_C(1) << request.number)))
                error = EIO;
            else
                *number = (int)request.number;
            break;
        }
        if (result != -LUME_EAGAIN && result != -LUME_EINTR)
        {
            error = result < 0 ? lume_errno_from_syscall((unsigned)-result) : EIO;
            break;
        }
        result = lume_native_call(LUME_SYS_SLEEP, 1, 0, 0);
        if (result > 0)
        {
            error = EIO;
            break;
        }
        if (result < 0 && result != -LUME_EINTR)
        {
            error = lume_errno_from_syscall((unsigned)-result);
            break;
        }
    }
done:
    errno = saved;
    return error;
}
