/* SPDX-License-Identifier: BSD-3-Clause */
#include <signal.h>
#include <errno.h>
#include <stdint.h>
#include "lume/abi.h"
#include "lume/signal.h"
#include "lume/process.h"
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
    struct lume_system_features features = {0};
    int64_t result = lume_native_call(LUME_SYS_FEATURES, (uintptr_t)&features, sizeof(features),
                                      LUME_FEATURE_VERSION);
    if (result || !(features.features & LUME_FEATURE_SIGNAL_WAIT))
    {
        error = result < 0 ? lume_errno_from_syscall((unsigned)-result) : result ? EIO : ENOSYS;
        goto done;
    }
    do
        result = lume_native_call(LUME_SYS_SIGNAL, LUME_SIGNAL_WAIT, selected, 0);
    while (result == -LUME_EINTR);
    if (result < 0)
        error = lume_errno_from_syscall((unsigned)-result);
    else if (!result || result >= NSIG || !(selected & (UINT32_C(1) << result)))
        error = EIO;
    else
        *number = (int)result;
done:
    errno = saved;
    return error;
}
