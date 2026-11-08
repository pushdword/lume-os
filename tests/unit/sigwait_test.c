/* SPDX-License-Identifier: BSD-3-Clause */
#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "lume/abi.h"
#include "lume/signal.h"
#include "lume/process.h"

static const uint32_t selected = (UINT32_C(1) << SIGIO) | (UINT32_C(1) << SIGUSR1);
static int64_t replies[8], feature_result;
static unsigned queries, waits;
static bool supported;

int lume_errno_from_syscall(unsigned error)
{
    return error == LUME_ENOSYS ? ENOSYS : error == LUME_EINVAL ? EINVAL : EIO;
}

int64_t lume_abi_host_call(unsigned operation, uint64_t a, uint64_t b, uint64_t c)
{
    errno = ERANGE;
    if (operation == LUME_SYS_FEATURES)
    {
        assert(a && b == sizeof(struct lume_system_features) && c == LUME_FEATURE_VERSION);
        ++queries;
        struct lume_system_features *features = (void *)(uintptr_t)a;
        *features =
            (struct lume_system_features){.features = supported ? LUME_FEATURE_SIGNAL_WAIT : 0};
        return feature_result;
    }
    assert(operation == LUME_SYS_SIGNAL && a == LUME_SIGNAL_WAIT && b == selected && !c &&
           waits < 8);
    return replies[waits++];
}

static void reset(void)
{
    queries = waits = 0;
    feature_result = 0;
    supported = true;
    for (unsigned i = 0; i < 8; ++i)
        replies[i] = SIGIO;
    errno = EBUSY;
}

int main(void)
{
    sigset_t set = selected;
    int number = -1;
    reset();
    replies[0] = -LUME_EINTR;
    assert(sigwait(&set, &number) == 0 && number == SIGIO && errno == EBUSY);
    assert(queries == 1 && waits == 2);
    reset();
    supported = false;
    number = -1;
    assert(sigwait(&set, &number) == ENOSYS && number == -1 && errno == EBUSY && !waits);
    reset();
    feature_result = -LUME_ENOSYS;
    assert(sigwait(&set, &number) == ENOSYS && number == -1 && errno == EBUSY && !waits);
    reset();
    replies[0] = -LUME_ENOSYS;
    assert(sigwait(&set, &number) == ENOSYS && number == -1 && errno == EBUSY && waits == 1);
    reset();
    replies[0] = -LUME_EINVAL;
    assert(sigwait(&set, &number) == EINVAL && number == -1 && errno == EBUSY);
    const int64_t malformed[] = {0, SIGUSR2, NSIG, INT64_MAX};
    for (unsigned i = 0; i < sizeof(malformed) / sizeof(*malformed); ++i)
    {
        reset();
        replies[0] = malformed[i];
        assert(sigwait(&set, &number) == EIO && number == -1 && errno == EBUSY);
    }
    reset();
    assert(sigwait(NULL, &number) == EFAULT && errno == EBUSY && !queries);
    assert(sigwait(&set, NULL) == EFAULT && errno == EBUSY && !queries);
    const uint32_t invalid[] = {0, 1, UINT32_C(1) << SIGKILL, UINT32_C(1) << SIGSTOP};
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(*invalid); ++i)
    {
        set = invalid[i];
        assert(sigwait(&set, &number) == EINVAL && errno == EBUSY && !queries);
    }
    puts("PASS_SIGWAIT event wait, compatibility, interrupted retry, zero sleep and errno");
    return 0;
}
