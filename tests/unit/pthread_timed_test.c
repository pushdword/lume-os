/* SPDX-License-Identifier: BSD-3-Clause */
#include "lume/pthread_timed.h"
#include "lume/thread.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

static int immediate, steps, waits, statuses[4];
static uint64_t deadline;
static pthread_mutex_t *waiting;

pthread_t pthread_self(void)
{
    return 7;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    assert(mutex == waiting);
    return immediate;
}

int64_t lume_thread_call(unsigned operation, struct lume_thread_request *request)
{
    assert(operation == LUME_THREAD_WAIT_REALTIME);
    assert(request->address == (uintptr_t)&waiting->word && request->value == 2);
    assert(request->timeout_ms == deadline);
    assert(waiting->word == 2 && waits < steps);
    int status = statuses[waits++];
    if (!status)
    {
        waiting->owner = 0;
        waiting->word = 0;
    }
    return status;
}

static void reset(pthread_mutex_t *mutex)
{
    *mutex = (pthread_mutex_t){.word = 1, .depth = 1, .owner = 11};
    waiting = mutex;
    immediate = EBUSY;
    waits = 0;
    steps = 0;
}

int main(void)
{
    pthread_mutex_t mutex;
    struct timespec end = {.tv_sec = 10, .tv_nsec = 1};
    reset(&mutex);
    immediate = 0;
    assert(!pthread_mutex_timedlock(&mutex, NULL) && !waits);
    reset(&mutex);
    mutex.type = PTHREAD_MUTEX_ERRORCHECK;
    mutex.owner = 7;
    assert(pthread_mutex_timedlock(&mutex, &end) == EDEADLK && !waits);
    reset(&mutex);
    end.tv_nsec = 1000000000;
    assert(pthread_mutex_timedlock(&mutex, &end) == EINVAL && !waits);
    end.tv_nsec = 1;
    deadline = 10001;
    statuses[0] = -EINTR;
    statuses[1] = -EAGAIN;
    statuses[2] = 0;
    steps = 3;
    assert(!pthread_mutex_timedlock(&mutex, &end));
    assert(waits == 3 && mutex.owner == 7 && mutex.depth == 1 && mutex.word == 2);
    reset(&mutex);
    statuses[0] = -ETIMEDOUT;
    steps = 1;
    assert(pthread_mutex_timedlock(&mutex, &end) == ETIMEDOUT);
    assert(waits == 1 && mutex.owner == 11 && mutex.depth == 1 && mutex.word == 2);
    reset(&mutex);
    end.tv_sec = -1;
    deadline = 0;
    steps = 1;
    assert(pthread_mutex_timedlock(&mutex, &end) == ETIMEDOUT && waits == 1);
    reset(&mutex);
    end.tv_sec = INT64_MAX;
    assert(pthread_mutex_timedlock(&mutex, &end) == EOVERFLOW && !waits);
    puts("PASS timed mutex deadlines, immediate acquisition, wake retries and timeout ownership");
    return 0;
}
