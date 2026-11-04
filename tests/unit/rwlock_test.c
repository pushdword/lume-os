#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include "../../libc/posix/pthread.c"

_Static_assert(sizeof(pthread_rwlock_t) == 56, "rwlock ABI size");
_Static_assert(offsetof(pthread_rwlock_t, writer) == 48, "rwlock ABI writer offset");

static struct rwlock_read_hold saved_holds[4][64];
static unsigned waits, wakes, recursive_reads, rejected_readers, interrupted;
static pthread_rwlock_t *active;
static int wait_error;
static bool park_reader;

static void enter(unsigned thread)
{
    assert(thread && thread < 4);
    if (identity)
        memcpy(saved_holds[identity], rwlock_read_holds, sizeof(rwlock_read_holds));
    identity = thread;
    memcpy(rwlock_read_holds, saved_holds[thread], sizeof(rwlock_read_holds));
}

int64_t lume_thread_call(unsigned operation, struct lume_thread_request *request)
{
    if (operation == LUME_THREAD_SELF)
        return identity ? identity : 1;
    if (operation == LUME_THREAD_WAKE)
    {
        ++wakes;
        return 0;
    }
    assert(operation == LUME_THREAD_WAIT);
    ++waits;
    assert(request->address == (uintptr_t)&active->changed.sequence);
    assert(active->waiters == 1 && active->changed.waiters == 1);
    assert(active->mutex.reserved == 1 && !active->writer && active->readers == 1);
    if (wait_error)
    {
        if (park_reader)
        {
            /* A new reader parks behind the writer while its wait releases the mutex. */
            ++active->waiters;
            ++active->changed.waiters;
        }
        return -wait_error;
    }
    if (interrupted)
    {
        --interrupted;
        return -EINTR;
    }
    for (unsigned i = 0; i < 10000; ++i)
    {
        enter(2);
        assert(pthread_rwlock_tryrdlock(active) == EBUSY);
        ++rejected_readers;
        enter(1);
        assert(!pthread_rwlock_rdlock(active));
        ++recursive_reads;
        assert(!pthread_rwlock_unlock(active));
        assert(active->readers == 1 && active->mutex.reserved == 1);
    }
    assert(!wakes);
    enter(1);
    assert(!pthread_rwlock_unlock(active));
    enter(3);
    return 0;
}

static void reset(void)
{
    identity = 0;
    memset(saved_holds, 0, sizeof(saved_holds));
    memset(rwlock_read_holds, 0, sizeof(rwlock_read_holds));
    waits = wakes = recursive_reads = rejected_readers = interrupted = 0;
    wait_error = 0;
    park_reader = false;
    enter(1);
}

static void try_contention(void)
{
    pthread_rwlock_t lock = PTHREAD_RWLOCK_INITIALIZER;
    reset();
    assert(!pthread_mutex_lock(&lock.mutex));
    enter(2);
    assert(pthread_rwlock_tryrdlock(&lock) == EBUSY);
    assert(pthread_rwlock_trywrlock(&lock) == EBUSY);
    assert(!waits && !wakes);
    enter(1);
    assert(!pthread_mutex_unlock(&lock.mutex));
    assert(!pthread_rwlock_rdlock(&lock));
    enter(2);
    assert(!pthread_rwlock_tryrdlock(&lock));
    assert(!pthread_rwlock_unlock(&lock));
    assert(pthread_rwlock_trywrlock(&lock) == EBUSY);
    assert(pthread_rwlock_unlock(&lock) == EPERM);
    enter(1);
    assert(!pthread_rwlock_unlock(&lock));
    assert(!pthread_rwlock_wrlock(&lock));
    assert(pthread_rwlock_rdlock(&lock) == EDEADLK);
    assert(pthread_rwlock_wrlock(&lock) == EDEADLK);
    assert(pthread_rwlock_tryrdlock(&lock) == EBUSY);
    assert(pthread_rwlock_trywrlock(&lock) == EBUSY);
    enter(2);
    assert(pthread_rwlock_unlock(&lock) == EPERM);
    enter(1);
    assert(!pthread_rwlock_unlock(&lock));
    assert(!waits && !wakes);
    assert(!pthread_rwlock_destroy(&lock));
}

static void writer_progress(unsigned signals)
{
    pthread_rwlock_t lock = PTHREAD_RWLOCK_INITIALIZER;
    reset();
    active = &lock;
    interrupted = signals;
    assert(!pthread_rwlock_rdlock(&lock));
    enter(3);
    assert(!pthread_rwlock_wrlock(&lock));
    assert(waits == signals + 1 && wakes == 1);
    assert(recursive_reads == 10000 && rejected_readers == 10000);
    assert(lock.writer == 3 && !lock.readers && !lock.waiters && !lock.mutex.reserved &&
           !lock.changed.waiters);
    assert(!pthread_rwlock_unlock(&lock));
    assert(wakes == 1 && !pthread_rwlock_destroy(&lock));
}

static void wait_failure(int error, bool reader_waiting)
{
    pthread_rwlock_t lock = PTHREAD_RWLOCK_INITIALIZER;
    reset();
    active = &lock;
    wait_error = error;
    park_reader = reader_waiting;
    assert(!pthread_rwlock_rdlock(&lock));
    enter(3);
    assert(pthread_rwlock_wrlock(&lock) == error);
    assert(waits == 1 && !lock.mutex.reserved);
    assert(lock.waiters == reader_waiting && lock.changed.waiters == reader_waiting);
    assert(wakes == reader_waiting);
    if (reader_waiting)
    {
        /* The awakened reader removes its wait registration before acquiring the read lock. */
        --lock.waiters;
        --lock.changed.waiters;
    }
    enter(2);
    assert(!pthread_rwlock_tryrdlock(&lock));
    assert(!pthread_rwlock_unlock(&lock));
    enter(1);
    assert(!pthread_rwlock_unlock(&lock));
    assert(wakes == reader_waiting && !pthread_rwlock_destroy(&lock));
}

static void reader_capacity(void)
{
    pthread_rwlock_t locks[65] = {0};
    reset();
    for (unsigned i = 0; i < 64; ++i)
        assert(!pthread_rwlock_tryrdlock(locks + i));
    assert(pthread_rwlock_tryrdlock(locks + 64) == EAGAIN);
    assert(pthread_rwlock_rdlock(locks + 64) == EAGAIN);
    assert(!pthread_rwlock_rdlock(locks));
    assert(locks[0].readers == 2);
    assert(!pthread_rwlock_unlock(locks));
    assert(!pthread_rwlock_unlock(locks));
    assert(!pthread_rwlock_tryrdlock(locks + 64));
    for (unsigned i = 1; i < 65; ++i)
        assert(!pthread_rwlock_unlock(locks + i));
    for (unsigned i = 0; i < 65; ++i)
        assert(!pthread_rwlock_destroy(locks + i));
    assert(!waits && !wakes);
}

int main(void)
{
    try_contention();
    writer_progress(0);
    writer_progress(1);
    wait_failure(EINVAL, false);
    wait_failure(ECANCELED, false);
    wait_failure(ECANCELED, true);
    reader_capacity();
    puts("PASS rwlock nonblocking try, writer progress, recursive readers and waiter cleanup");
    return 0;
}
