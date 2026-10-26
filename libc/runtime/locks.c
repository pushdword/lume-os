#include <stdbool.h>
#include <stddef.h>
#include "pthread.h"
#include <sys/lock.h>

struct __lock
{
    pthread_mutex_t mutex;
    unsigned used;
};
struct __lock __lock___libc_recursive_mutex = {.mutex = {.type = PTHREAD_MUTEX_RECURSIVE},
                                               .used = 1};
static struct __lock locks[32];

void __retarget_lock_init(_LOCK_T *out)
{
    for (unsigned i = 0; i < 32; ++i)
    {
        unsigned expected = 0;
        if (__atomic_compare_exchange_n(&locks[i].used, &expected, 1, false, __ATOMIC_ACQUIRE,
                                        __ATOMIC_RELAXED))
        {
            pthread_mutex_init(&locks[i].mutex, NULL);
            *out = &locks[i];
            return;
        }
    }
    __builtin_trap();
}

void __retarget_lock_init_recursive(_LOCK_T *out)
{
    __retarget_lock_init(out);
    (*out)->mutex.type = PTHREAD_MUTEX_RECURSIVE;
}

void __retarget_lock_close(_LOCK_T lock)
{
    __atomic_store_n(&lock->used, 0, __ATOMIC_RELEASE);
}

void __retarget_lock_close_recursive(_LOCK_T lock)
{
    __retarget_lock_close(lock);
}

void __retarget_lock_acquire(_LOCK_T lock)
{
    (void)pthread_mutex_lock(&lock->mutex);
}

void __retarget_lock_acquire_recursive(_LOCK_T lock)
{
    __retarget_lock_acquire(lock);
}

void __retarget_lock_release(_LOCK_T lock)
{
    (void)pthread_mutex_unlock(&lock->mutex);
}

void __retarget_lock_release_recursive(_LOCK_T lock)
{
    __retarget_lock_release(lock);
}
