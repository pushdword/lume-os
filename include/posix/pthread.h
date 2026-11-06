#ifndef LUME_PTHREAD_H
#define LUME_PTHREAD_H
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <sched.h>
typedef uint64_t pthread_t;
typedef unsigned pthread_key_t;

typedef struct
{
    size_t stacksize, guardsize;
    int detachstate;
} pthread_attr_t;

typedef struct
{
    uint32_t word, type, depth, reserved;
    pthread_t owner __attribute__((aligned(8)));
} pthread_mutex_t;

typedef struct
{
    int type;
} pthread_mutexattr_t;

typedef struct
{
    uint32_t sequence, waiters;
    int clock;
} pthread_cond_t;

typedef struct
{
    int clock;
} pthread_condattr_t;

typedef struct
{
    pthread_mutex_t mutex;
    pthread_cond_t changed;
    unsigned readers, waiters;
    pthread_t writer;
} pthread_rwlock_t;

typedef struct
{
    int shared;
} pthread_rwlockattr_t;

typedef uint32_t pthread_once_t;
#define PTHREAD_CREATE_JOINABLE 0
#define PTHREAD_CREATE_DETACHED 1
#define PTHREAD_MUTEX_NORMAL 0
#define PTHREAD_MUTEX_RECURSIVE 1
#define PTHREAD_MUTEX_ERRORCHECK 2
#define PTHREAD_MUTEX_DEFAULT PTHREAD_MUTEX_NORMAL
#define PTHREAD_MUTEX_INITIALIZER {0}
#define PTHREAD_COND_INITIALIZER {0, 0, CLOCK_REALTIME}
#define PTHREAD_ONCE_INIT 0
#define PTHREAD_RWLOCK_INITIALIZER {0}
#define PTHREAD_PROCESS_PRIVATE 0
#define PTHREAD_PROCESS_SHARED 1
#define PTHREAD_DESTRUCTOR_ITERATIONS 4
#define PTHREAD_STACK_MIN 32768u

struct __lume_cleanup_record
{
    struct __lume_cleanup_record *previous;
    void (*routine)(void *);
    void *argument;
};

/* Cleanup records remain in the lexical scope shared by the paired macros. */
#define pthread_cleanup_push(routine, argument)                                                    \
    do                                                                                             \
    {                                                                                              \
        struct __lume_cleanup_record __lume_cleanup;                                               \
        __lume_cleanup_push(&__lume_cleanup, (routine), (argument));
#define pthread_cleanup_pop(execute)                                                               \
    __lume_cleanup_pop(&__lume_cleanup, (execute));                                                \
    }                                                                                              \
    while (0)
#ifdef __cplusplus
extern "C"
{
#endif
    int pthread_create(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);
    int pthread_join(pthread_t, void **);
    int pthread_detach(pthread_t);
    pthread_t pthread_self(void);
    int pthread_equal(pthread_t, pthread_t);
    int pthread_atfork(void (*)(void), void (*)(void), void (*)(void));
    void __lume_cleanup_push(struct __lume_cleanup_record *, void (*)(void *), void *);
    void __lume_cleanup_pop(struct __lume_cleanup_record *, int);
#ifdef __cplusplus
    [[noreturn]] void pthread_exit(void *);
#else
_Noreturn void pthread_exit(void *);
#endif
    int pthread_attr_init(pthread_attr_t *);
    int pthread_attr_destroy(pthread_attr_t *);
    int pthread_attr_setstacksize(pthread_attr_t *, size_t);
    int pthread_attr_getstacksize(const pthread_attr_t *, size_t *);
    int pthread_attr_getstack(const pthread_attr_t *, void **, size_t *);
    int pthread_attr_setstack(pthread_attr_t *, void *, size_t);
    int pthread_getattr_np(pthread_t, pthread_attr_t *);
    int pthread_getname_np(pthread_t, char *, size_t);
    int pthread_setname_np(pthread_t, const char *);
    int pthread_getschedparam(pthread_t, int *, struct sched_param *);
    int pthread_setschedparam(pthread_t, int, const struct sched_param *);
    int pthread_attr_setdetachstate(pthread_attr_t *, int);
    int pthread_attr_getdetachstate(const pthread_attr_t *, int *);
    int pthread_attr_setguardsize(pthread_attr_t *, size_t);
    int pthread_attr_getguardsize(const pthread_attr_t *, size_t *);
    int pthread_mutexattr_init(pthread_mutexattr_t *);
    int pthread_mutexattr_destroy(pthread_mutexattr_t *);
    int pthread_mutexattr_settype(pthread_mutexattr_t *, int);
    int pthread_mutexattr_gettype(const pthread_mutexattr_t *, int *);
    int pthread_mutex_init(pthread_mutex_t *, const pthread_mutexattr_t *);
    int pthread_mutex_destroy(pthread_mutex_t *);
    int pthread_mutex_lock(pthread_mutex_t *);
    int pthread_mutex_trylock(pthread_mutex_t *);
    int pthread_mutex_unlock(pthread_mutex_t *);
    int pthread_condattr_init(pthread_condattr_t *);
    int pthread_condattr_destroy(pthread_condattr_t *);
    int pthread_condattr_setclock(pthread_condattr_t *, clockid_t);
    int pthread_condattr_getclock(const pthread_condattr_t *, clockid_t *);
    int pthread_cond_init(pthread_cond_t *, const pthread_condattr_t *);
    int pthread_cond_destroy(pthread_cond_t *);
    int pthread_cond_wait(pthread_cond_t *, pthread_mutex_t *);
    int pthread_cond_timedwait(pthread_cond_t *, pthread_mutex_t *, const struct timespec *);
    int pthread_cond_signal(pthread_cond_t *);
    int pthread_cond_broadcast(pthread_cond_t *);
    int pthread_rwlockattr_init(pthread_rwlockattr_t *);
    int pthread_rwlockattr_destroy(pthread_rwlockattr_t *);
    int pthread_rwlockattr_getpshared(const pthread_rwlockattr_t *, int *);
    int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *, int);
    int pthread_rwlock_init(pthread_rwlock_t *, const pthread_rwlockattr_t *);
    int pthread_rwlock_destroy(pthread_rwlock_t *);
    int pthread_rwlock_rdlock(pthread_rwlock_t *);
    int pthread_rwlock_tryrdlock(pthread_rwlock_t *);
    int pthread_rwlock_wrlock(pthread_rwlock_t *);
    int pthread_rwlock_trywrlock(pthread_rwlock_t *);
    int pthread_rwlock_unlock(pthread_rwlock_t *);
    int pthread_once(pthread_once_t *, void (*)(void));
    int pthread_key_create(pthread_key_t *, void (*)(void *));
    int pthread_key_delete(pthread_key_t);
    int pthread_setspecific(pthread_key_t, const void *);
    void *pthread_getspecific(pthread_key_t);
#ifdef __cplusplus
}
#endif
#endif
