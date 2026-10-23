#include "lume/abi.h"
#include "lume/thread.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "pthread.h"
#include "errno_map.h"
#include <sys/lock.h>

int64_t lume_thread_call(unsigned operation, struct lume_thread_request *request)
{
    register uint64_t x8 __asm__("x8") = LUME_SYS_THREAD;
    register uint64_t x0 __asm__("x0") = operation;
    register uint64_t x1 __asm__("x1") = (uintptr_t)request;
    register uint64_t x2 __asm__("x2") = request ? sizeof(*request) : 0;
    __asm__ volatile("svc #0x4c55" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2) : "memory", "cc");
    int64_t result = (int64_t)x0;
    return result < 0 ? -(int64_t)lume_errno_from_syscall((unsigned)-result) : result;
}

extern unsigned char __tls_start[], __tls_file_end[], __tls_size[], __tls_align[];
static _Alignas(64) unsigned char initial_tls[65536 + 64];

void lume_tls_init(void)
{
    size_t align = (uintptr_t)__tls_align;
    size_t bias = (16 + align - 1) & ~(align - 1);
    for (size_t i = 0; i < (size_t)(__tls_file_end - __tls_start); ++i)
        initial_tls[bias + i] = __tls_start[i];
    __asm__ volatile("msr tpidr_el0, %0; isb" ::"r"(initial_tls) : "memory");
}

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
