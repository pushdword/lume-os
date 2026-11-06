/* SPDX-License-Identifier: BSD-3-Clause */
#include <semaphore.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include "lume/thread.h"
#include "absolute_time.h"

extern int64_t lume_thread_call(unsigned, struct lume_thread_request *);

#define SEMAPHORE_READY UINT32_C(0x53454d31)
#define SEMAPHORE_WAITER (UINT64_C(1) << 32)

_Static_assert(__atomic_always_lock_free(sizeof(uint64_t), 0), "semaphore counter atomics");

/* The low word holds tokens and the high word counts registered waiters. A post
 * publishes its token and captures the waiter count in one atomic operation. */

static int failure(int error)
{
    errno = error;
    return -1;
}

static bool ready(const sem_t *semaphore)
{
    return semaphore && __atomic_load_n(&semaphore->state, __ATOMIC_ACQUIRE) == SEMAPHORE_READY;
}

static int take(sem_t *semaphore)
{
    if (!ready(semaphore))
        return EINVAL;
    uint64_t counters = __atomic_load_n(&semaphore->counters, __ATOMIC_RELAXED);
    while ((uint32_t)counters)
        if (__atomic_compare_exchange_n(&semaphore->counters, &counters, counters - 1, false,
                                        __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
            return 0;
    return EAGAIN;
}

static void wake(uintptr_t address)
{
    struct lume_thread_request request = {.address = address, .value = 1};
    (void)lume_thread_call(LUME_THREAD_WAKE, &request);
}

int sem_init(sem_t *semaphore, int shared, unsigned value)
{
    if (!semaphore || value > SEM_VALUE_MAX)
        return failure(EINVAL);
    if (shared)
        return failure(ENOTSUP);
    __atomic_store_n(&semaphore->counters, value, __ATOMIC_RELAXED);
    __atomic_store_n(&semaphore->state, SEMAPHORE_READY, __ATOMIC_RELEASE);
    return 0;
}

int sem_destroy(sem_t *semaphore)
{
    if (!ready(semaphore))
        return failure(EINVAL);
    if (__atomic_load_n(&semaphore->counters, __ATOMIC_ACQUIRE) >> 32)
        return failure(EBUSY);
    __atomic_store_n(&semaphore->state, 0, __ATOMIC_RELEASE);
    return 0;
}

int sem_trywait(sem_t *semaphore)
{
    int result = take(semaphore);
    return result ? failure(result) : 0;
}

static int wait_slow(sem_t *semaphore, bool realtime, uint64_t deadline)
{
    uintptr_t address = (uintptr_t)&semaphore->counters;
    uint64_t counters = __atomic_load_n(&semaphore->counters, __ATOMIC_RELAXED);
    do
    {
        if ((counters >> 32) == UINT32_MAX)
            return failure(EAGAIN);
    } while (!__atomic_compare_exchange_n(&semaphore->counters, &counters,
                                          counters + SEMAPHORE_WAITER, false, __ATOMIC_ACQ_REL,
                                          __ATOMIC_RELAXED));
    int result;
    for (;;)
    {
        result = take(semaphore);
        if (result != EAGAIN)
            break;
        struct lume_thread_request request = {.address = address, .timeout_ms = deadline};
        int64_t status =
            lume_thread_call(realtime ? LUME_THREAD_WAIT_REALTIME : LUME_THREAD_WAIT, &request);
        if (status == -EAGAIN)
        {
            if (realtime)
            {
                struct timespec current;
                if (clock_gettime(CLOCK_REALTIME, &current))
                {
                    result = errno;
                    break;
                }
            }
            continue;
        }
        if (status < 0)
        {
            result = (int)-status;
            if (result == ETIMEDOUT)
            {
                int available = take(semaphore);
                if (available != EAGAIN)
                    result = available;
            }
            break;
        }
    }
    uint64_t remaining =
        __atomic_sub_fetch(&semaphore->counters, SEMAPHORE_WAITER, __ATOMIC_ACQ_REL);
    if ((remaining >> 32) && (uint32_t)remaining)
        wake(address);
    return result ? failure(result) : 0;
}

int sem_wait(sem_t *semaphore)
{
    int result = take(semaphore);
    if (result != EAGAIN)
        return result ? failure(result) : 0;
    return wait_slow(semaphore, false, UINT64_MAX);
}

int sem_timedwait(sem_t *semaphore, const struct timespec *absolute)
{
    int result = take(semaphore);
    if (result != EAGAIN)
        return result ? failure(result) : 0;
    uint64_t deadline;
    result = timespec_deadline_ms(absolute, &deadline);
    if (result)
        return failure(result);
    struct timespec current;
    if (clock_gettime(CLOCK_REALTIME, &current))
        return -1;
    return wait_slow(semaphore, true, deadline);
}

int sem_post(sem_t *semaphore)
{
    if (!ready(semaphore))
        return failure(EINVAL);
    uintptr_t address = (uintptr_t)&semaphore->counters;
    uint64_t counters = __atomic_load_n(&semaphore->counters, __ATOMIC_RELAXED);
    do
    {
        if ((uint32_t)counters >= SEM_VALUE_MAX)
            return failure(EOVERFLOW);
    } while (!__atomic_compare_exchange_n(&semaphore->counters, &counters, counters + 1, false,
                                          __ATOMIC_RELEASE, __ATOMIC_RELAXED));
    if (counters >> 32)
        wake(address);
    return 0;
}

int sem_getvalue(sem_t *semaphore, int *value)
{
    if (!ready(semaphore) || !value)
        return failure(EINVAL);
    *value = (int)(uint32_t)__atomic_load_n(&semaphore->counters, __ATOMIC_ACQUIRE);
    return 0;
}

sem_t *sem_open(const char *name, int flags, ...)
{
    (void)name;
    (void)flags;
    errno = ENOSYS;
    return SEM_FAILED;
}

int sem_close(sem_t *semaphore)
{
    (void)semaphore;
    return failure(ENOSYS);
}

int sem_unlink(const char *name)
{
    (void)name;
    return failure(ENOSYS);
}
