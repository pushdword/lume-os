#include "pthread.h"
#include "lume/thread.h"
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

#include "absolute_time.h"

#if defined(LUME_PTHREAD_COMPAT32)
/* Legacy request buffers require thread stacks below the 32-bit address limit. */
#define THREAD_STACK_DEFAULT LUME_PTHREAD_COMPAT_STACK_DEFAULT
#define THREAD_STACK_MAX LUME_PTHREAD_COMPAT_STACK_MAX
#else
#define THREAD_STACK_DEFAULT LUME_PTHREAD_STACK_DEFAULT
#define THREAD_STACK_MAX LUME_PTHREAD_STACK_MAX
#endif

extern int64_t lume_thread_call(unsigned, struct lume_thread_request *);
extern void native_cxx_thread_finalize(void) __attribute__((weak));
static _Thread_local pthread_t identity;
static _Thread_local struct __lume_cleanup_record *cleanup_stack;
static pthread_t fork_identity, fork_native_identity;

static pthread_t native_thread(pthread_t thread)
{
    return fork_identity && thread == fork_identity ? fork_native_identity : thread;
}

void __lume_cleanup_push(struct __lume_cleanup_record *record, void (*routine)(void *),
                         void *argument)
{
    *record = (struct __lume_cleanup_record){
        .previous = cleanup_stack, .routine = routine, .argument = argument};
    cleanup_stack = record;
}

void __lume_cleanup_pop(struct __lume_cleanup_record *record, int execute)
{
    if (cleanup_stack != record)
        __builtin_trap();
    cleanup_stack = record->previous;
    if (execute)
        record->routine(record->argument);
}

#define KEY_SEGMENT_BITS 8u
#define KEY_SEGMENT_SIZE (1u << KEY_SEGMENT_BITS)
#define KEY_SLOT_BITS 16u
#define KEY_SLOT_MASK ((1u << KEY_SLOT_BITS) - 1u)
#define KEY_GENERATION_MAX UINT16_MAX

struct thread_key
{
    pthread_key_t active;
    uint16_t generation, next;
    void (*destroy)(void *);
};

struct thread_key_segment
{
    struct thread_key entries[KEY_SEGMENT_SIZE];
};

struct thread_value
{
    pthread_key_t key;
    unsigned destructor_pass;
    void *value;
};

struct thread_value_segment
{
    struct thread_value entries[KEY_SEGMENT_SIZE];
};

/* Published key segments remain allocated so concurrent getters never observe retired storage. */
static struct thread_key_segment *key_segments[KEY_SEGMENT_SIZE];
static unsigned key_unused = 1, key_free;
static _Thread_local struct thread_value_segment *key_values[KEY_SEGMENT_SIZE];
static _Thread_local unsigned destructor_pass;
static pthread_mutex_t key_lock = PTHREAD_MUTEX_INITIALIZER;

void lume_pthread_fork_prepare(void)
{
    pthread_mutex_lock(&key_lock);
}

void lume_pthread_fork_parent(void)
{
    pthread_mutex_unlock(&key_lock);
}

void lume_pthread_fork_child(void)
{
    key_lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    int64_t native = lume_thread_call(LUME_THREAD_SELF, NULL);
    if (native < 0)
        return;
    if (!identity)
        identity = (pthread_t)native;
    fork_identity = identity;
    fork_native_identity = (pthread_t)native;
}

static struct thread_start
{
    struct thread_start *next;
    bool detached, finished;
    pthread_t tid;
    void *(*function)(void *);
    void *argument;
} *starts;

static pthread_mutex_t start_lock = PTHREAD_MUTEX_INITIALIZER;
static _Thread_local struct thread_start *current_start;

static struct stack_attribute
{
    struct stack_attribute *next;
    const pthread_attr_t *attribute;
    void *base;
} *stack_attributes;

static pthread_mutex_t attribute_lock = PTHREAD_MUTEX_INITIALIZER;

static void release_stack_attribute(const pthread_attr_t *attribute)
{
    pthread_mutex_lock(&attribute_lock);
    struct stack_attribute **position = &stack_attributes;
    while (*position && (*position)->attribute != attribute)
        position = &(*position)->next;
    struct stack_attribute *entry = *position;
    if (entry)
        *position = entry->next;
    pthread_mutex_unlock(&attribute_lock);
    free(entry);
}

static struct thread_key *find_key(pthread_key_t key)
{
    unsigned slot = key & KEY_SLOT_MASK;
    if (!slot || !(key >> KEY_SLOT_BITS))
        return NULL;
    struct thread_key_segment *segment =
        __atomic_load_n(&key_segments[slot >> KEY_SEGMENT_BITS], __ATOMIC_ACQUIRE);
    if (!segment)
        return NULL;
    struct thread_key *entry = &segment->entries[slot & (KEY_SEGMENT_SIZE - 1)];
    return __atomic_load_n(&entry->active, __ATOMIC_ACQUIRE) == key ? entry : NULL;
}

static void remove_start(struct thread_start *start)
{
    struct thread_start **entry = &starts;
    while (*entry && *entry != start)
        entry = &(*entry)->next;
    if (*entry)
        *entry = start->next;
}

/* Each thread can hold read locks on 64 distinct objects. Recursion shares an entry.
 * An exhausted table returns EAGAIN without allocating or blocking. */
static _Thread_local struct rwlock_read_hold
{
    pthread_rwlock_t *lock;
    unsigned depth;
} rwlock_read_holds[64];

static struct rwlock_writer_wait
{
    struct rwlock_writer_wait *next;
    pthread_rwlock_t *lock;
} *rwlock_writers;

static pthread_mutex_t writer_queue_lock = PTHREAD_MUTEX_INITIALIZER;

static void queue_writer(struct rwlock_writer_wait *waiter, pthread_rwlock_t *lock)
{
    *waiter = (struct rwlock_writer_wait){.lock = lock};
    pthread_mutex_lock(&writer_queue_lock);
    struct rwlock_writer_wait **position = &rwlock_writers;
    while (*position)
        position = &(*position)->next;
    *position = waiter;
    pthread_mutex_unlock(&writer_queue_lock);
}

static bool first_writer(const struct rwlock_writer_wait *waiter)
{
    pthread_mutex_lock(&writer_queue_lock);
    const struct rwlock_writer_wait *position = rwlock_writers;
    while (position && position->lock != waiter->lock)
        position = position->next;
    bool first = position == waiter;
    pthread_mutex_unlock(&writer_queue_lock);
    return first;
}

static void unqueue_writer(struct rwlock_writer_wait *waiter)
{
    pthread_mutex_lock(&writer_queue_lock);
    struct rwlock_writer_wait **position = &rwlock_writers;
    while (*position && *position != waiter)
        position = &(*position)->next;
    if (*position)
        *position = waiter->next;
    pthread_mutex_unlock(&writer_queue_lock);
}

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

static _Noreturn void finish_thread(void *value, bool execute_cleanup)
{
    if (execute_cleanup)
        while (cleanup_stack)
            __lume_cleanup_pop(cleanup_stack, 1);
    else
        cleanup_stack = NULL;
    if (native_cxx_thread_finalize)
        native_cxx_thread_finalize();
    for (destructor_pass = 1; destructor_pass <= PTHREAD_DESTRUCTOR_ITERATIONS; ++destructor_pass)
        for (unsigned group = 0; group < KEY_SEGMENT_SIZE; ++group)
        {
            struct thread_value_segment *segment = key_values[group];
            if (!segment)
                continue;
            for (unsigned i = 0; i < KEY_SEGMENT_SIZE; ++i)
            {
                struct thread_value *entry = &segment->entries[i];
                if (!entry->key || entry->destructor_pass >= destructor_pass)
                    continue;
                pthread_mutex_lock(&key_lock);
                entry->destructor_pass = destructor_pass;
                struct thread_key *key = find_key(entry->key);
                void (*destroy)(void *) = key ? key->destroy : NULL;
                void *p = entry->value;
                entry->value = NULL;
                pthread_mutex_unlock(&key_lock);
                if (p && destroy)
                    destroy(p);
            }
        }
    for (unsigned group = 0; group < KEY_SEGMENT_SIZE; ++group)
    {
        struct thread_value_segment *segment = key_values[group];
        key_values[group] = NULL;
        free(segment);
    }
    struct thread_start *released = NULL;
    pthread_mutex_lock(&start_lock);
    if (current_start)
    {
        current_start->finished = true;
        if (current_start->detached)
        {
            remove_start(current_start);
            released = current_start;
        }
    }
    current_start = NULL;
    pthread_mutex_unlock(&start_lock);
    free(released);
    struct lume_thread_request r = {.value = (uintptr_t)value};
    (void)lume_thread_call(LUME_THREAD_EXIT, &r);
    for (;;)
        ;
}

_Noreturn void pthread_exit(void *value)
{
    finish_thread(value, true);
}

static _Noreturn void trampoline(void *argument)
{
    current_start = argument;
    void *value = current_start->function(current_start->argument);
    finish_thread(value, false);
}

int pthread_attr_init(pthread_attr_t *a)
{
    if (!a)
        return EINVAL;
    release_stack_attribute(a);
    *a = (pthread_attr_t){.stacksize = THREAD_STACK_DEFAULT, .guardsize = 4096};
    return 0;
}

int pthread_attr_destroy(pthread_attr_t *a)
{
    if (!a)
        return EINVAL;
    release_stack_attribute(a);
    return 0;
}

int pthread_attr_setstacksize(pthread_attr_t *a, size_t n)
{
    if (n < PTHREAD_STACK_MIN || n > THREAD_STACK_MAX)
        return EINVAL;
    a->stacksize = n;
    return 0;
}

int pthread_attr_getstacksize(const pthread_attr_t *a, size_t *n)
{
    *n = a->stacksize;
    return 0;
}

int pthread_attr_getstack(const pthread_attr_t *a, void **base, size_t *bytes)
{
    if (!a || !base || !bytes)
        return EINVAL;
    *base = NULL;
    *bytes = a->stacksize;
    pthread_mutex_lock(&attribute_lock);
    for (const struct stack_attribute *entry = stack_attributes; entry; entry = entry->next)
        if (entry->attribute == a)
        {
            *base = entry->base;
            break;
        }
    pthread_mutex_unlock(&attribute_lock);
    return 0;
}

int pthread_attr_setstack(pthread_attr_t *a, void *base, size_t bytes)
{
    (void)a;
    (void)base;
    (void)bytes;
    return ENOTSUP;
}

int pthread_getattr_np(pthread_t thread, pthread_attr_t *a)
{
    if (!a)
        return EINVAL;
    struct lume_thread_request request = {.tid = native_thread(thread)};
    int64_t result = lume_thread_call(LUME_THREAD_STACK, &request);
    if (result < 0)
        return (int)-result;
    struct stack_attribute *entry = malloc(sizeof(*entry));
    if (!entry)
        return ENOMEM;
    release_stack_attribute(a);
    *a = (pthread_attr_t){.stacksize = (size_t)request.stack_bytes,
                          .guardsize = (size_t)request.argument,
                          .detachstate =
                              request.value ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE};
    *entry = (struct stack_attribute){.attribute = a, .base = (void *)(uintptr_t)request.address};
    pthread_mutex_lock(&attribute_lock);
    entry->next = stack_attributes;
    stack_attributes = entry;
    pthread_mutex_unlock(&attribute_lock);
    return 0;
}

int pthread_getname_np(pthread_t thread, char *name, size_t bytes)
{
    if (!name || !bytes)
        return EINVAL;
    struct lume_thread_request request = {
        .tid = native_thread(thread), .address = (uintptr_t)name, .stack_bytes = bytes};
    int64_t result = lume_thread_call(LUME_THREAD_NAME, &request);
    return result < 0 ? (int)-result : 0;
}

int pthread_setname_np(pthread_t thread, const char *name)
{
    if (!name)
        return EINVAL;
    size_t bytes = strnlen(name, 16);
    if (bytes == 16)
        return ERANGE;
    struct lume_thread_request request = {.tid = native_thread(thread),
                                          .address = (uintptr_t)name,
                                          .stack_bytes = bytes + 1,
                                          .value = 1};
    int64_t result = lume_thread_call(LUME_THREAD_NAME, &request);
    return result < 0 ? (int)-result : 0;
}

int pthread_getschedparam(pthread_t thread, int *policy, struct sched_param *parameters)
{
    if (!policy || !parameters)
        return EINVAL;
    struct lume_thread_request request = {.tid = native_thread(thread)};
    int64_t result = lume_thread_call(LUME_THREAD_STACK, &request);
    if (result < 0)
        return (int)-result;
    *policy = SCHED_OTHER;
    *parameters = (struct sched_param){.sched_priority = 0};
    return 0;
}

int pthread_setschedparam(pthread_t thread, int policy, const struct sched_param *parameters)
{
    if (!parameters)
        return EINVAL;
    if (policy != SCHED_OTHER || parameters->sched_priority)
        return ENOTSUP;
    int actual_policy;
    struct sched_param actual;
    return pthread_getschedparam(thread, &actual_policy, &actual);
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
    if (a->stacksize < PTHREAD_STACK_MIN || a->stacksize > THREAD_STACK_MAX ||
        (a->detachstate != PTHREAD_CREATE_JOINABLE && a->detachstate != PTHREAD_CREATE_DETACHED))
        return EINVAL;
    if (a->guardsize != 4096)
        return ENOTSUP;
    void *supplied_stack;
    size_t supplied_bytes;
    int attributes = pthread_attr_getstack(a, &supplied_stack, &supplied_bytes);
    if (attributes || supplied_stack)
        return attributes ? attributes : ENOTSUP;
    struct thread_start *s = malloc(sizeof(*s));
    if (!s)
        return EAGAIN;
    *s = (struct thread_start){.function = run, .argument = argument};
    pthread_mutex_lock(&start_lock);
    s->next = starts;
    starts = s;
    struct lume_thread_request r = {
        .entry = (uintptr_t)trampoline, .argument = (uintptr_t)s, .stack_bytes = a->stacksize};
#if !defined(LUME_PTHREAD_COMPAT32) &&                                                             \
    (defined(__aarch64__) || defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64))
    r.value = LUME_THREAD_CREATE_NATIVE64;
#elif defined(LUME_ARCH_I686)
    r.value = LUME_THREAD_CREATE_NATIVE32;
#endif
    int64_t tid = lume_thread_call(LUME_THREAD_CREATE, &r);
    if (tid < 0)
        remove_start(s);
    else
    {
        s->tid = (pthread_t)tid;
        *out = s->tid;
    }
    pthread_mutex_unlock(&start_lock);
    if (tid < 0)
        free(s);
    if (tid >= 0 && a->detachstate == PTHREAD_CREATE_DETACHED)
        return pthread_detach((pthread_t)tid);
    return tid < 0 ? (int)-tid : 0;
}

int pthread_join(pthread_t thread, void **value)
{
    if (pthread_equal(thread, pthread_self()))
        return EDEADLK;
    struct lume_thread_request r = {.tid = native_thread(thread)};
    int64_t result;
    do
    {
        result = lume_thread_call(LUME_THREAD_JOIN, &r);
    } while (result == -EINTR);
    if (result < 0)
        return (int)-result;
    if (value)
        *value = (void *)(uintptr_t)r.value;
    struct thread_start *released = NULL;
    pthread_mutex_lock(&start_lock);
    for (struct thread_start *entry = starts; entry; entry = entry->next)
        if (entry->tid == thread)
        {
            remove_start(entry);
            released = entry;
            break;
        }
    pthread_mutex_unlock(&start_lock);
    free(released);
    return 0;
}

int pthread_detach(pthread_t thread)
{
    struct thread_start *released = NULL;
    pthread_mutex_lock(&start_lock);
    struct lume_thread_request r = {.tid = native_thread(thread)};
    int64_t result = lume_thread_call(LUME_THREAD_DETACH, &r);
    if (result >= 0)
        for (struct thread_start *entry = starts; entry; entry = entry->next)
            if (entry->tid == thread)
            {
                entry->detached = true;
                if (entry->finished)
                {
                    remove_start(entry);
                    released = entry;
                }
                break;
            }
    pthread_mutex_unlock(&start_lock);
    free(released);
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

static int condition_wait(pthread_cond_t *c, pthread_mutex_t *m, uint64_t ms, bool realtime)
{
    uint32_t sequence = __atomic_load_n(&c->sequence, __ATOMIC_ACQUIRE);
    __atomic_add_fetch(&c->waiters, 1, __ATOMIC_RELAXED);
    int result = pthread_mutex_unlock(m);
    if (!result)
    {
        if (realtime)
        {
            struct lume_thread_request request = {
                .address = (uintptr_t)&c->sequence, .value = sequence, .timeout_ms = ms};
            int64_t status = lume_thread_call(LUME_THREAD_WAIT_REALTIME, &request);
            result = status < 0 ? (int)-status : 0;
        }
        else
            result = wait_word(&c->sequence, sequence, ms);
        if (result == EAGAIN || result == EINTR)
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
    return condition_wait(c, m, UINT64_MAX, false);
}

int pthread_cond_timedwait(pthread_cond_t *c, pthread_mutex_t *m, const struct timespec *end)
{
    if (!end || end->tv_nsec < 0 || end->tv_nsec >= 1000000000)
        return EINVAL;
    struct timespec now;
    if (clock_gettime(c->clock, &now))
        return errno;
    if (c->clock == CLOCK_REALTIME)
    {
        uint64_t deadline;
        int error = timespec_deadline_ms(end, &deadline);
        return error ? error : condition_wait(c, m, deadline, true);
    }
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
    return condition_wait(c, m, ms, false);
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

int pthread_rwlockattr_init(pthread_rwlockattr_t *a)
{
    a->shared = PTHREAD_PROCESS_PRIVATE;
    return 0;
}

int pthread_rwlockattr_destroy(pthread_rwlockattr_t *a)
{
    (void)a;
    return 0;
}

int pthread_rwlockattr_getpshared(const pthread_rwlockattr_t *a, int *shared)
{
    *shared = a->shared;
    return 0;
}

int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *a, int shared)
{
    if (shared == PTHREAD_PROCESS_SHARED)
        return ENOTSUP;
    if (shared != PTHREAD_PROCESS_PRIVATE)
        return EINVAL;
    a->shared = shared;
    return 0;
}

int pthread_rwlock_init(pthread_rwlock_t *lock, const pthread_rwlockattr_t *a)
{
    if (a && a->shared != PTHREAD_PROCESS_PRIVATE)
        return ENOTSUP;
    *lock = (pthread_rwlock_t)PTHREAD_RWLOCK_INITIALIZER;
    return 0;
}

int pthread_rwlock_destroy(pthread_rwlock_t *lock)
{
    int result = pthread_mutex_trylock(&lock->mutex);
    if (result)
        return result;
    bool busy = lock->writer || lock->readers || lock->waiters || lock->mutex.reserved;
    pthread_mutex_unlock(&lock->mutex);
    if (busy)
        return EBUSY;
    result = pthread_cond_destroy(&lock->changed);
    return result ? result : pthread_mutex_destroy(&lock->mutex);
}

static struct rwlock_read_hold *rwlock_read_slot(pthread_rwlock_t *lock)
{
    struct rwlock_read_hold *available = NULL;
    for (unsigned i = 0; i < sizeof(rwlock_read_holds) / sizeof(rwlock_read_holds[0]); ++i)
    {
        struct rwlock_read_hold *hold = rwlock_read_holds + i;
        if (hold->lock == lock)
            return hold;
        if (!hold->lock && !available)
            available = hold;
    }
    return available;
}

static void rwlock_wake(pthread_rwlock_t *lock)
{
    if (lock->waiters && __atomic_load_n(&lock->changed.waiters, __ATOMIC_ACQUIRE))
        pthread_cond_broadcast(&lock->changed);
}

/* Writers enter in queue order. Existing readers may still recurse. */
static int rwlock_acquire(pthread_rwlock_t *lock, bool writer, bool try_only)
{
    int result = try_only ? pthread_mutex_trylock(&lock->mutex) : pthread_mutex_lock(&lock->mutex);
    if (result)
        return result;
    pthread_t self = pthread_self();
    struct rwlock_read_hold *hold = writer ? NULL : rwlock_read_slot(lock);
    bool recursive = hold && hold->lock == lock;
    bool pending_writer = false;
    struct rwlock_writer_wait waiter;
    if (!writer && !hold)
        result = EAGAIN;
    if (!try_only && lock->writer == self)
        result = EDEADLK;
    if (!result && writer && !try_only && (lock->writer || lock->readers || lock->mutex.reserved))
    {
        if (lock->mutex.reserved == UINT_MAX)
            result = EAGAIN;
        else
        {
            queue_writer(&waiter, lock);
            ++lock->mutex.reserved;
            pending_writer = true;
        }
    }
    while (!result &&
           (lock->writer ||
            (writer && (lock->readers ||
                        (pending_writer ? !first_writer(&waiter) : lock->mutex.reserved != 0))) ||
            (!writer && !recursive && lock->mutex.reserved)))
    {
        if (try_only)
        {
            result = EBUSY;
            break;
        }
        if (lock->waiters == UINT_MAX)
        {
            result = EAGAIN;
            break;
        }
        ++lock->waiters;
        result = pthread_cond_wait(&lock->changed, &lock->mutex);
        --lock->waiters;
        if (result == EINTR)
            result = 0;
    }
    if (pending_writer)
    {
        unqueue_writer(&waiter);
        --lock->mutex.reserved;
    }
    if (!result)
    {
        if (writer)
            lock->writer = self;
        else if (lock->readers == UINT_MAX)
            result = EAGAIN;
        else
        {
            ++lock->readers;
            hold->lock = lock;
            ++hold->depth;
        }
    }
    if (pending_writer && result && !lock->writer)
        rwlock_wake(lock);
    pthread_mutex_unlock(&lock->mutex);
    return result;
}

int pthread_rwlock_rdlock(pthread_rwlock_t *lock)
{
    return rwlock_acquire(lock, false, false);
}

int pthread_rwlock_tryrdlock(pthread_rwlock_t *lock)
{
    return rwlock_acquire(lock, false, true);
}

int pthread_rwlock_wrlock(pthread_rwlock_t *lock)
{
    return rwlock_acquire(lock, true, false);
}

int pthread_rwlock_trywrlock(pthread_rwlock_t *lock)
{
    return rwlock_acquire(lock, true, true);
}

int pthread_rwlock_unlock(pthread_rwlock_t *lock)
{
    int result = pthread_mutex_lock(&lock->mutex);
    if (result)
        return result;
    if (lock->writer)
    {
        if (lock->writer != pthread_self())
            result = EPERM;
        else
            lock->writer = 0;
    }
    else if (lock->readers)
    {
        struct rwlock_read_hold *hold = rwlock_read_slot(lock);
        if (!hold || hold->lock != lock)
            result = EPERM;
        else
        {
            --lock->readers;
            if (!--hold->depth)
                hold->lock = NULL;
        }
    }
    else
        result = EPERM;
    if (!result && !lock->readers && !lock->writer)
        rwlock_wake(lock);
    pthread_mutex_unlock(&lock->mutex);
    return result;
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
    if (!out)
        return EINVAL;
    struct thread_key_segment *allocated = NULL;
    for (;;)
    {
        pthread_mutex_lock(&key_lock);
        unsigned slot = key_free ? key_free : key_unused;
        if (slot > KEY_SLOT_MASK)
        {
            pthread_mutex_unlock(&key_lock);
            free(allocated);
            return EAGAIN;
        }
        unsigned group = slot >> KEY_SEGMENT_BITS;
        struct thread_key_segment *segment = key_segments[group];
        if (!segment && !allocated)
        {
            pthread_mutex_unlock(&key_lock);
            allocated = malloc(sizeof(*allocated));
            if (!allocated)
                return EAGAIN;
            memset(allocated, 0, sizeof(*allocated));
            continue;
        }
        if (!segment)
        {
            segment = allocated;
            allocated = NULL;
            __atomic_store_n(&key_segments[group], segment, __ATOMIC_RELEASE);
        }
        struct thread_key *entry = &segment->entries[slot & (KEY_SEGMENT_SIZE - 1)];
        if (key_free)
            key_free = entry->next;
        else
            ++key_unused;
        ++entry->generation;
        pthread_key_t key = (pthread_key_t)entry->generation << KEY_SLOT_BITS | slot;
        entry->destroy = destroy;
        __atomic_store_n(&entry->active, key, __ATOMIC_RELEASE);
        *out = key;
        pthread_mutex_unlock(&key_lock);
        free(allocated);
        return 0;
    }
}

int pthread_key_delete(pthread_key_t key)
{
    pthread_mutex_lock(&key_lock);
    struct thread_key *entry = find_key(key);
    if (!entry)
    {
        pthread_mutex_unlock(&key_lock);
        return EINVAL;
    }
    __atomic_store_n(&entry->active, 0, __ATOMIC_RELEASE);
    entry->destroy = NULL;
    if (entry->generation < KEY_GENERATION_MAX)
    {
        entry->next = (uint16_t)key_free;
        key_free = key & KEY_SLOT_MASK;
    }
    pthread_mutex_unlock(&key_lock);
    return 0;
}

int pthread_setspecific(pthread_key_t key, const void *value)
{
    struct thread_value_segment *allocated = NULL;
    pthread_mutex_lock(&key_lock);
    if (!find_key(key))
    {
        pthread_mutex_unlock(&key_lock);
        return EINVAL;
    }
    unsigned slot = key & KEY_SLOT_MASK, group = slot >> KEY_SEGMENT_BITS;
    struct thread_value_segment *segment = key_values[group];
    if (!segment && !value)
    {
        pthread_mutex_unlock(&key_lock);
        return 0;
    }
    if (!segment)
    {
        pthread_mutex_unlock(&key_lock);
        allocated = malloc(sizeof(*allocated));
        if (!allocated)
            return ENOMEM;
        memset(allocated, 0, sizeof(*allocated));
        pthread_mutex_lock(&key_lock);
        if (!find_key(key))
        {
            pthread_mutex_unlock(&key_lock);
            free(allocated);
            return EINVAL;
        }
        segment = key_values[group];
        if (!segment)
        {
            key_values[group] = segment = allocated;
            allocated = NULL;
        }
    }
    struct thread_value *entry = &segment->entries[slot & (KEY_SEGMENT_SIZE - 1)];
    if (entry->key != key)
        *entry = (struct thread_value){.key = key, .destructor_pass = destructor_pass};
    entry->value = (void *)value;
    pthread_mutex_unlock(&key_lock);
    free(allocated);
    return 0;
}

void *pthread_getspecific(pthread_key_t key)
{
    struct thread_key *descriptor = find_key(key);
    if (!descriptor)
        return NULL;
    unsigned slot = key & KEY_SLOT_MASK;
    struct thread_value_segment *segment = key_values[slot >> KEY_SEGMENT_BITS];
    if (!segment)
        return NULL;
    struct thread_value *entry = &segment->entries[slot & (KEY_SEGMENT_SIZE - 1)];
    void *value = entry->key == key ? entry->value : NULL;
    return __atomic_load_n(&descriptor->active, __ATOMIC_ACQUIRE) == key ? value : NULL;
}
