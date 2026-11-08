#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <limits.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../fixtures/pthread_host_namespace.h"

/* Keep the host thread types separate from the runtime contract. */
#define pthread_t lume_test_pthread_t
#define pthread_key_t lume_test_pthread_key_t
#define pthread_attr_t lume_test_pthread_attr_t
#define pthread_mutex_t lume_test_pthread_mutex_t
#define pthread_mutexattr_t lume_test_pthread_mutexattr_t
#define pthread_cond_t lume_test_pthread_cond_t
#define pthread_condattr_t lume_test_pthread_condattr_t
#define pthread_rwlock_t lume_test_pthread_rwlock_t
#define pthread_rwlockattr_t lume_test_pthread_rwlockattr_t
#define pthread_once_t lume_test_pthread_once_t
#undef PTHREAD_STACK_MIN
#include "../../libc/posix/pthread.c"

static struct lume_thread_request created;
static unsigned creates, joins, stack_queries;
static int create_error;

int64_t lume_thread_call(unsigned operation, struct lume_thread_request *request)
{
    if (operation == LUME_THREAD_SELF)
        return 1;
    if (operation == LUME_THREAD_WAKE)
        return 0;
    assert(request);
    if (operation == LUME_THREAD_CREATE)
    {
        ++creates;
        created = *request;
        return create_error ? -create_error : 2;
    }
    if (operation == LUME_THREAD_STACK)
    {
        ++stack_queries;
        assert(request->tid == 2);
        request->address = created.value ? UINT64_C(0x100004000) : UINT64_C(0x5a004000);
        request->stack_bytes = created.stack_bytes;
        request->argument = 4096;
        return 0;
    }
    assert(operation == LUME_THREAD_JOIN && request->tid == 2);
    ++joins;
    request->value = UINT64_C(0x12345678);
    return 0;
}

static void *entry(void *argument)
{
    return argument;
}

static void create_and_join(const pthread_attr_t *attributes, size_t expected)
{
    pthread_t thread;
    assert(!starts);
    assert(!pthread_create(&thread, attributes, entry, (void *)(uintptr_t)17));
    assert(thread == 2 && starts && created.entry && created.argument);
    assert(created.stack_bytes == expected);
#if defined(LUME_PTHREAD_COMPAT32) || defined(LUME_ARCH_I686)
    assert(created.value == 0);
#else
    assert(created.value == LUME_THREAD_CREATE_NATIVE64);
#endif
    assert(!created.tid && !created.address && !created.timeout_ms);
    pthread_attr_t actual;
    assert(!pthread_getattr_np(thread, &actual));
    void *base;
    size_t bytes, guard;
    assert(!pthread_attr_getstack(&actual, &base, &bytes));
    assert(!pthread_attr_getguardsize(&actual, &guard));
    assert(bytes == expected && guard == 4096 && !((uintptr_t)base & 4095));
    assert((created.value != 0) == ((uintptr_t)base > UINT32_MAX));
    assert(!pthread_attr_destroy(&actual));
    void *result;
    assert(!pthread_join(thread, &result));
    assert((uintptr_t)result == UINT64_C(0x12345678) && !starts && !stack_attributes);
}

int main(void)
{
    pthread_attr_t attributes;
    assert(!pthread_attr_init(&attributes));
    size_t initial;
    assert(!pthread_attr_getstacksize(&attributes, &initial));
#if defined(LUME_PTHREAD_COMPAT32) || defined(LUME_ARCH_I686)
    assert(initial == 128u * 1024u && THREAD_STACK_MAX == 768u * 1024u);
    assert(pthread_attr_setstacksize(&attributes, 2u * 1024u * 1024u) == EINVAL);
    assert(attributes.stacksize == initial);
#else
    assert(initial == 2u * 1024u * 1024u && THREAD_STACK_MAX == 8u * 1024u * 1024u);
    assert(!pthread_attr_setstacksize(&attributes, 2u * 1024u * 1024u));
    create_and_join(&attributes, 2u * 1024u * 1024u);
#endif
    create_and_join(NULL, initial);
    assert(!pthread_attr_setstacksize(&attributes, THREAD_STACK_MAX));
    create_and_join(&attributes, THREAD_STACK_MAX);
    assert(pthread_attr_setstacksize(&attributes, (size_t)THREAD_STACK_MAX + 1) == EINVAL);
    assert(pthread_attr_setstacksize(&attributes, PTHREAD_STACK_MIN - 1) == EINVAL);
    assert(attributes.stacksize == THREAD_STACK_MAX);
    assert(!pthread_attr_setstacksize(&attributes, PTHREAD_STACK_MIN));
    create_and_join(&attributes, PTHREAD_STACK_MIN);
    create_error = ENOMEM;
    pthread_t thread;
    assert(pthread_create(&thread, &attributes, entry, NULL) == ENOMEM);
    assert(!starts && !stack_attributes && creates == joins + 1 && stack_queries == joins);
    assert(!pthread_attr_destroy(&attributes));
    puts(
        "PASS_PTHREAD_STACK Native64 and compatibility limits, create requests, queries and join cleanup");
    return 0;
}
