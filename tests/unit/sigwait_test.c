/* SPDX-License-Identifier: BSD-3-Clause */
#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include "lume/abi.h"
#include "lume/signal.h"

static const uint32_t selected = (UINT32_C(1) << SIGIO) | (UINT32_C(1) << SIGUSR1);
static int64_t replies[8], sleeps[8];
static unsigned tries, waits;
static unsigned returned_signal;

int lume_errno_from_syscall(unsigned error)
{
    return error == LUME_ENOSYS ? ENOSYS : error == LUME_EINVAL ? EINVAL : EIO;
}

int64_t lume_abi_host_call(unsigned operation, uint64_t a, uint64_t b, uint64_t c)
{
    errno = ERANGE;
    if (operation == LUME_SYS_SLEEP)
    {
        assert(a == 1 && !b && !c && waits < 8);
        return sleeps[waits++];
    }
    assert(operation == LUME_SYS_SIGNAL && a == LUME_SIGNAL_TRY_WAIT &&
           c == sizeof(struct lume_signal_request) && tries < 8);
    struct lume_signal_request *request = (void *)(uintptr_t)b;
    assert(request->version == LUME_SIGNAL_VERSION && request->bytes == sizeof(*request) &&
           request->mask == selected && !request->number && !request->flags && !request->handler &&
           !request->restorer && !request->delay_ms && !request->interval_ms &&
           !request->operation && !request->pid && !request->group);
    int64_t result = replies[tries++];
    if (!result)
        request->number = returned_signal;
    return result;
}

static void reset(void)
{
    tries = waits = 0;
    for (unsigned i = 0; i < 8; ++i)
        replies[i] = sleeps[i] = 0;
    returned_signal = SIGIO;
    errno = EBUSY;
}

int main(void)
{
    sigset_t set = selected;
    int number = -1;
    reset();
    replies[0] = -LUME_EAGAIN;
    replies[1] = -LUME_EINTR;
    sleeps[0] = -LUME_EINTR;
    assert(sigwait(&set, &number) == 0 && number == SIGIO && errno == EBUSY);
    assert(tries == 3 && waits == 2);
    reset();
    replies[0] = -LUME_ENOSYS;
    number = -1;
    assert(sigwait(&set, &number) == ENOSYS && number == -1 && errno == EBUSY && !waits);
    reset();
    replies[0] = -LUME_EINVAL;
    assert(sigwait(&set, &number) == EINVAL && number == -1 && errno == EBUSY);
    reset();
    replies[0] = -LUME_EAGAIN;
    sleeps[0] = -LUME_ENOSYS;
    assert(sigwait(&set, &number) == ENOSYS && number == -1 && errno == EBUSY);
    reset();
    returned_signal = SIGUSR2;
    assert(sigwait(&set, &number) == EIO && number == -1 && errno == EBUSY);
    reset();
    replies[0] = 1;
    assert(sigwait(&set, &number) == EIO && number == -1 && errno == EBUSY);
    reset();
    assert(sigwait(NULL, &number) == EFAULT && errno == EBUSY && !tries);
    assert(sigwait(&set, NULL) == EFAULT && errno == EBUSY && !tries);
    const uint32_t invalid[] = {0, 1, UINT32_C(1) << SIGKILL, UINT32_C(1) << SIGSTOP};
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
    {
        set = invalid[i];
        assert(sigwait(&set, &number) == EINVAL && errno == EBUSY && !tries);
    }
    puts("PASS_SIGWAIT retry, sleep, interruptions, error results and errno preservation");
    return 0;
}
