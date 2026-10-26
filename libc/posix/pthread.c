#include "pthread.h"
#include "lume/thread.h"
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

extern int64_t lume_thread_call(unsigned, struct lume_thread_request *);
static _Thread_local pthread_t identity;
static _Thread_local void *key_values[16];
static _Thread_local unsigned key_generations[16];

static struct
{
    unsigned generation;
    bool used;
    void (*destroy)(void *);
} keys[16];

static pthread_mutex_t key_lock = PTHREAD_MUTEX_INITIALIZER;

static struct thread_start
{
    unsigned used, detached, finished;
    pthread_t tid;
    void *(*function)(void *);
    void *argument;
} starts[8];

static pthread_mutex_t start_lock = PTHREAD_MUTEX_INITIALIZER;
static _Thread_local struct thread_start *current_start;

static int wait_word(uint32_t *word, uint32_t value, uint64_t timeout)
{
    struct lume_thread_request r = {
        .address = (uintptr_t)word, .value = value, .timeout_ms = timeout};
    int64_t result = lume_thread_call(LUME_THREAD_WAIT, &r);
    return result < 0 ? (int)-result : 0;
}

static void wake_word(uint32_t *word, unsigned count)
{
    struct lume_thread_request r = {.address = (uintptr_t)word, .value = count};
    (void)lume_thread_call(LUME_THREAD_WAKE, &r);
}

pthread_t pthread_self(void)
{
    if (!identity)
        identity = (pthread_t)lume_thread_call(LUME_THREAD_SELF, NULL);
    return identity;
}

int pthread_equal(pthread_t a, pthread_t b)
{
    return a == b;
}

_Noreturn void pthread_exit(void *value)
{
    for (unsigned pass = 0; pass < PTHREAD_DESTRUCTOR_ITERATIONS; ++pass)
        for (unsigned i = 0; i < 16; ++i)
        {
            pthread_mutex_lock(&key_lock);
            void (*destroy)(void *) =
                keys[i].used && keys[i].generation == key_generations[i] ? keys[i].destroy : NULL;
            void *p = key_values[i];
            key_values[i] = NULL;
            pthread_mutex_unlock(&key_lock);
            if (p && destroy)
                destroy(p);
        }
    pthread_mutex_lock(&start_lock);
    if (current_start)
    {
        current_start->finished = 1;
        if (current_start->detached)
            current_start->used = 0;
    }
    pthread_mutex_unlock(&start_lock);
    struct lume_thread_request r = {.value = (uintptr_t)value};
    (void)lume_thread_call(LUME_THREAD_EXIT, &r);
    for (;;)
        ;
}

static _Noreturn void trampoline(void *argument)
{
    current_start = argument;
    pthread_exit(current_start->function(current_start->argument));
}

int pthread_attr_init(pthread_attr_t *a)
{
    *a = (pthread_attr_t){.stacksize = 128 * 1024, .guardsize = 4096};
    return 0;
}

int pthread_attr_destroy(pthread_attr_t *a)
{
    (void)a;
    return 0;
}

int pthread_attr_setstacksize(pthread_attr_t *a, size_t n)
{
    if (n < PTHREAD_STACK_MIN || n > 768 * 1024)
        return EINVAL;
    a->stacksize = n;
    return 0;
}

int pthread_attr_getstacksize(const pthread_attr_t *a, size_t *n)
{
    *n = a->stacksize;
    return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t *a, int n)
{
    if (n != PTHREAD_CREATE_JOINABLE && n != PTHREAD_CREATE_DETACHED)
        return EINVAL;
    a->detachstate = n;
    return 0;
}

int pthread_attr_getdetachstate(const pthread_attr_t *a, int *n)
{
    *n = a->detachstate;
    return 0;
}

int pthread_attr_setguardsize(pthread_attr_t *a, size_t n)
{
    if (n != 4096)
        return ENOTSUP;
    a->guardsize = n;
    return 0;
}

int pthread_attr_getguardsize(const pthread_attr_t *a, size_t *n)
{
    *n = a->guardsize;
    return 0;
}

int pthread_create(pthread_t *out, const pthread_attr_t *a, void *(*run)(void *), void *argument)
{
    if (!out || !run)
        return EINVAL;
    pthread_attr_t defaults;
    pthread_attr_init(&defaults);
    if (!a)
        a = &defaults;
    pthread_mutex_lock(&start_lock);
    struct thread_start *s = NULL;
    for (unsigned i = 0; i < 8; ++i)
        if (!__atomic_load_n(&starts[i].used, __ATOMIC_ACQUIRE))
        {
            s = &starts[i];
            *s = (struct thread_start){.used = 1, .function = run, .argument = argument};
            break;
        }
    if (!s)
    {
        pthread_mutex_unlock(&start_lock);
        return EAGAIN;
    }
    struct lume_thread_request r = {
        .entry = (uintptr_t)trampoline, .argument = (uintptr_t)s, .stack_bytes = a->stacksize};
    int64_t tid = lume_thread_call(LUME_THREAD_CREATE, &r);
    if (tid < 0)
        s->used = 0;
    else
    {
        s->tid = (pthread_t)tid;
        *out = s->tid;
    }
    pthread_mutex_unlock(&start_lock);
    if (tid >= 0 && a->detachstate == PTHREAD_CREATE_DETACHED)
        return pthread_detach((pthread_t)tid);
    return tid < 0 ? (int)-tid : 0;
}

int pthread_join(pthread_t thread, void **value)
{
    if (pthread_equal(thread, pthread_self()))
        return EDEADLK;
    struct lume_thread_request r = {.tid = thread};
    int64_t result = lume_thread_call(LUME_THREAD_JOIN, &r);
    if (result < 0)
        return (int)-result;
    if (value)
        *value = (void *)(uintptr_t)r.value;
    pthread_mutex_lock(&start_lock);
    for (unsigned i = 0; i < 8; ++i)
        if (starts[i].used && starts[i].tid == thread)
            starts[i].used = 0;
    pthread_mutex_unlock(&start_lock);
    return 0;
}

int pthread_detach(pthread_t thread)
{
    pthread_mutex_lock(&start_lock);
    struct lume_thread_request r = {.tid = thread};
    int64_t result = lume_thread_call(LUME_THREAD_DETACH, &r);
    if (result >= 0)
        for (unsigned i = 0; i < 8; ++i)
            if (starts[i].used && starts[i].tid == thread)
            {
                starts[i].detached = 1;
                if (starts[i].finished)
                    starts[i].used = 0;
            }
    pthread_mutex_unlock(&start_lock);
    return result < 0 ? (int)-result : 0;
}

int pthread_mutexattr_init(pthread_mutexattr_t *a)
{
    a->type = 0;
    return 0;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t *a)
{
    (void)a;
    return 0;
}

int pthread_mutexattr_gettype(const pthread_mutexattr_t *a, int *type)
{
    *type = a->type;
    return 0;
}

int pthread_mutexattr_settype(pthread_mutexattr_t *a, int type)
{
    if (type < 0 || type > PTHREAD_MUTEX_ERRORCHECK)
        return EINVAL;
    a->type = type;
    return 0;
}

int pthread_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t *a)
{
    *m = (pthread_mutex_t){.type = a ? (unsigned)a->type : 0};
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *m)
{
    return __atomic_load_n(&m->word, __ATOMIC_ACQUIRE) ? EBUSY : 0;
}

int pthread_mutex_trylock(pthread_mutex_t *m)
{
    pthread_t self = pthread_self();
    if (m->type && __atomic_load_n(&m->owner, __ATOMIC_RELAXED) == self)
    {
        if (m->type != PTHREAD_MUTEX_RECURSIVE)
            return EBUSY;
        if (m->depth == UINT_MAX)
            return EAGAIN;
        ++m->depth;
        return 0;
    }
    uint32_t expected = 0;
    if (!__atomic_compare_exchange_n(&m->word, &expected, 1, false, __ATOMIC_ACQUIRE,
                                     __ATOMIC_RELAXED))
        return EBUSY;
    __atomic_store_n(&m->owner, self, __ATOMIC_RELAXED);
    m->depth = 1;
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t *m)
{
    if (m->type == PTHREAD_MUTEX_ERRORCHECK &&
        __atomic_load_n(&m->owner, __ATOMIC_RELAXED) == pthread_self())
        return EDEADLK;
    int result;
    result = pthread_mutex_trylock(m);
    if (result != EBUSY)
        return result;
    for (;;)
    {
        if (!__atomic_exchange_n(&m->word, 2, __ATOMIC_ACQUIRE))
        {
            __atomic_store_n(&m->owner, pthread_self(), __ATOMIC_RELAXED);
            m->depth = 1;
            return 0;
        }
        (void)wait_word(&m->word, 2, UINT64_MAX);
    }
    return result;
}

int pthread_mutex_unlock(pthread_mutex_t *m)
{
    if (__atomic_load_n(&m->owner, __ATOMIC_RELAXED) != pthread_self())
        return EPERM;
    if (--m->depth)
        return 0;
    __atomic_store_n(&m->owner, 0, __ATOMIC_RELAXED);
    if (__atomic_exchange_n(&m->word, 0, __ATOMIC_RELEASE) == 2)
        wake_word(&m->word, 1);
    return 0;
}

int pthread_condattr_init(pthread_condattr_t *a)
{
    a->clock = CLOCK_REALTIME;
    return 0;
}

int pthread_condattr_destroy(pthread_condattr_t *a)
{
    (void)a;
    return 0;
}

int pthread_condattr_setclock(pthread_condattr_t *a, clockid_t clock)
{
    if (clock != CLOCK_MONOTONIC && clock != CLOCK_REALTIME)
        return EINVAL;
    a->clock = clock;
    return 0;
}

int pthread_condattr_getclock(const pthread_condattr_t *a, clockid_t *clock)
{
    *clock = a->clock;
    return 0;
}

int pthread_cond_init(pthread_cond_t *c, const pthread_condattr_t *a)
{
    *c = (pthread_cond_t){.clock = a ? a->clock : CLOCK_REALTIME};
    return 0;
}

int pthread_cond_destroy(pthread_cond_t *c)
{
    return __atomic_load_n(&c->waiters, __ATOMIC_ACQUIRE) ? EBUSY : 0;
}

static int condition_wait(pthread_cond_t *c, pthread_mutex_t *m, uint64_t ms)
{
    uint32_t sequence = __atomic_load_n(&c->sequence, __ATOMIC_ACQUIRE);
    __atomic_add_fetch(&c->waiters, 1, __ATOMIC_RELAXED);
    int result = pthread_mutex_unlock(m);
    if (!result)
    {
        result = wait_word(&c->sequence, sequence, ms);
        if (result == EAGAIN)
            result = 0;
        int lock = pthread_mutex_lock(m);
        if (lock)
            result = lock;
    }
    __atomic_sub_fetch(&c->waiters, 1, __ATOMIC_RELEASE);
    return result;
}

int pthread_cond_wait(pthread_cond_t *c, pthread_mutex_t *m)
{
    return condition_wait(c, m, UINT64_MAX);
}

int pthread_cond_timedwait(pthread_cond_t *c, pthread_mutex_t *m, const struct timespec *end)
{
    if (!end || end->tv_sec < 0 || end->tv_nsec < 0 || end->tv_nsec >= 1000000000)
        return EINVAL;
    struct timespec now;
    if (clock_gettime(c->clock, &now))
        return ENOTSUP;
    uint64_t ms = 0;
    if (end->tv_sec > now.tv_sec || (end->tv_sec == now.tv_sec && end->tv_nsec > now.tv_nsec))
    {
        uint64_t seconds = (uint64_t)end->tv_sec - (uint64_t)now.tv_sec;
        if (seconds > (UINT64_MAX - 1000) / 1000)
            return EOVERFLOW;
        int64_t ns = end->tv_nsec - now.tv_nsec;
        ms = seconds * 1000;
        if (ns >= 0)
            ms += (uint64_t)(ns + 999999) / 1000000;
        else
            ms -= (uint64_t)-ns / 1000000;
    }
    return condition_wait(c, m, ms);
}

int pthread_cond_signal(pthread_cond_t *c)
{
    __atomic_add_fetch(&c->sequence, 1, __ATOMIC_RELEASE);
    wake_word(&c->sequence, 1);
    return 0;
}

int pthread_cond_broadcast(pthread_cond_t *c)
{
    __atomic_add_fetch(&c->sequence, 1, __ATOMIC_RELEASE);
    wake_word(&c->sequence, UINT_MAX);
    return 0;
}

int pthread_once(pthread_once_t *once, void (*function)(void))
{
    uint32_t expected = 0;
    if (__atomic_compare_exchange_n(once, &expected, 1, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
    {
        function();
        __atomic_store_n(once, 2, __ATOMIC_RELEASE);
        wake_word(once, UINT_MAX);
    }
    else
        while (__atomic_load_n(once, __ATOMIC_ACQUIRE) != 2)
            (void)wait_word(once, 1, UINT64_MAX);
    return 0;
}

int pthread_key_create(pthread_key_t *out, void (*destroy)(void *))
{
    pthread_mutex_lock(&key_lock);
    for (unsigned i = 0; i < 16; ++i)
        if (!keys[i].used)
        {
            keys[i].used = true;
            keys[i].destroy = destroy;
            *out = ++keys[i].generation * 16 + i;
            pthread_mutex_unlock(&key_lock);
            return 0;
        }
    pthread_mutex_unlock(&key_lock);
    return EAGAIN;
}

int pthread_key_delete(pthread_key_t key)
{
    pthread_mutex_lock(&key_lock);
    unsigned i = key % 16;
    int result = !keys[i].used || keys[i].generation != key / 16 ? EINVAL : 0;
    if (!result)
        keys[i].used = false;
    pthread_mutex_unlock(&key_lock);
    return result;
}

int pthread_setspecific(pthread_key_t key, const void *value)
{
    pthread_mutex_lock(&key_lock);
    unsigned i = key % 16;
    int result = !keys[i].used || keys[i].generation != key / 16 ? EINVAL : 0;
    if (!result)
    {
        key_values[i] = (void *)value;
        key_generations[i] = keys[i].generation;
    }
    pthread_mutex_unlock(&key_lock);
    return result;
}

void *pthread_getspecific(pthread_key_t key)
{
    pthread_mutex_lock(&key_lock);
    unsigned i = key % 16;
    void *value =
        keys[i].used && keys[i].generation == key / 16 && keys[i].generation == key_generations[i]
            ? key_values[i]
            : NULL;
    pthread_mutex_unlock(&key_lock);
    return value;
}
