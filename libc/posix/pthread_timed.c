/* SPDX-License-Identifier: BSD-3-Clause */
#include "pthread_timed.h"
#include "lume/thread.h"
#include "absolute_time.h"
#include <errno.h>
#include <stdint.h>

extern int64_t lume_thread_call(unsigned, struct lume_thread_request *);

int pthread_mutex_timedlock(pthread_mutex_t *mutex, const struct timespec *end)
{
    pthread_t self = pthread_self();
    if (mutex->type == PTHREAD_MUTEX_ERRORCHECK &&
        __atomic_load_n(&mutex->owner, __ATOMIC_RELAXED) == self)
        return EDEADLK;
    int result = pthread_mutex_trylock(mutex);
    if (result != EBUSY)
        return result;
    uint64_t deadline;
    result = timespec_deadline_ms(end, &deadline);
    if (result)
        return result;
    for (;;)
    {
        if (!__atomic_exchange_n(&mutex->word, 2, __ATOMIC_ACQUIRE))
        {
            __atomic_store_n(&mutex->owner, self, __ATOMIC_RELAXED);
            mutex->depth = 1;
            return 0;
        }
        struct lume_thread_request request = {
            .address = (uintptr_t)&mutex->word, .value = 2, .timeout_ms = deadline};
        int64_t status = lume_thread_call(LUME_THREAD_WAIT_REALTIME, &request);
        if (status < 0 && status != -EAGAIN && status != -EINTR)
            return (int)-status;
    }
}
