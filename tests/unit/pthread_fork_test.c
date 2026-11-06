/* SPDX-License-Identifier: BSD-3-Clause */
#define _POSIX_C_SOURCE 200809L
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include "../../libc/posix/pthread.c"

static _Thread_local int64_t native_identity;
static pthread_t expected_native, selected_tid;
static unsigned targeted_calls;
static pthread_key_t inherited_key;
static int inherited_value;

int64_t lume_thread_call(unsigned operation, struct lume_thread_request *request)
{
    if (operation == LUME_THREAD_SELF)
        return native_identity;
    if (operation == LUME_THREAD_WAKE)
        return 0;
    assert(request);
    assert(operation == LUME_THREAD_STACK || operation == LUME_THREAD_NAME ||
           operation == LUME_THREAD_JOIN || operation == LUME_THREAD_DETACH);
    selected_tid = request->tid;
    assert(selected_tid == expected_native);
    ++targeted_calls;
    if (operation == LUME_THREAD_STACK)
    {
        request->address = UINT64_C(0x100004000);
        request->stack_bytes = 131072;
        request->argument = 4096;
        request->value = 0;
        return 0;
    }
    if (operation == LUME_THREAD_NAME)
    {
        char *name = (void *)(uintptr_t)request->address;
        if (request->value)
            assert(!strcmp(name, "renamed"));
        else
        {
            assert(request->stack_bytes >= sizeof("inherited"));
            memcpy(name, "inherited", sizeof("inherited"));
        }
        return 0;
    }
    return -ESRCH;
}

static void inspect_thread(pthread_t logical)
{
    pthread_attr_t attributes;
    assert(!pthread_getattr_np(logical, &attributes));
    void *base;
    size_t bytes;
    assert(!pthread_attr_getstack(&attributes, &base, &bytes));
    assert((uintptr_t)base == UINT64_C(0x100004000) && bytes == 131072);
    assert(!pthread_attr_destroy(&attributes));
    char name[16];
    assert(!pthread_getname_np(logical, name, sizeof(name)));
    assert(!strcmp(name, "inherited"));
    assert(!pthread_setname_np(logical, "renamed"));
    int policy;
    struct sched_param parameters;
    assert(!pthread_getschedparam(logical, &policy, &parameters));
    assert(policy == SCHED_OTHER && !parameters.sched_priority);
    assert(!pthread_setschedparam(logical, SCHED_OTHER, &parameters));
}

static DWORD WINAPI secondary(void *argument)
{
    pthread_t logical = (pthread_t)(uintptr_t)argument;
    native_identity = UINT64_C(0x3303);
    assert(!identity && !cleanup_stack && !current_start);
    assert(pthread_self() == (pthread_t)native_identity);
    assert(!pthread_getspecific(inherited_key));
    inspect_thread(logical);
    assert(pthread_join(logical, NULL) == ESRCH);
    assert(pthread_detach(logical) == ESRCH);
    return 0;
}

static void inspect_from_secondary(pthread_t logical)
{
    HANDLE thread = CreateThread(NULL, 0, secondary, (void *)(uintptr_t)logical, 0, NULL);
    assert(thread);
    assert(WaitForSingleObject(thread, 10000) == WAIT_OBJECT_0);
    DWORD result;
    assert(GetExitCodeThread(thread, &result) && !result);
    assert(CloseHandle(thread));
}

static void cleanup(void *argument)
{
    (void)argument;
    assert(0);
}

int main(void)
{
    native_identity = UINT64_C(0x1101);
    pthread_t logical = pthread_self();
    assert(logical == (pthread_t)native_identity);
    pthread_mutex_t mutex;
    pthread_mutexattr_t type;
    assert(!pthread_mutexattr_init(&type));
    assert(!pthread_mutexattr_settype(&type, PTHREAD_MUTEX_RECURSIVE));
    assert(!pthread_mutex_init(&mutex, &type));
    assert(!pthread_mutex_lock(&mutex) && !pthread_mutex_lock(&mutex));
    assert(!pthread_key_create(&inherited_key, NULL));
    assert(!pthread_setspecific(inherited_key, &inherited_value));
    struct __lume_cleanup_record record;
    __lume_cleanup_push(&record, cleanup, &inherited_value);
    struct thread_start start = {.tid = logical};
    current_start = &start;

    native_identity = UINT64_C(0x2202);
    lume_pthread_fork_child();
    expected_native = (pthread_t)native_identity;
    assert(pthread_self() == logical && mutex.owner == logical && mutex.depth == 2);
    assert(cleanup_stack == &record && current_start == &start);
    assert(pthread_getspecific(inherited_key) == &inherited_value);
    assert(!pthread_mutex_unlock(&mutex) && !pthread_mutex_unlock(&mutex));
    inspect_thread(logical);
    unsigned before = targeted_calls;
    assert(pthread_join(logical, NULL) == EDEADLK && targeted_calls == before);
    inspect_from_secondary(logical);

    assert(!pthread_mutex_lock(&mutex));
    native_identity = UINT64_C(0x5505);
    lume_pthread_fork_child();
    expected_native = (pthread_t)native_identity;
    assert(pthread_self() == logical && mutex.owner == logical);
    assert(!pthread_mutex_unlock(&mutex));
    inspect_from_secondary(logical);
    expected_native = UINT64_C(0x4404);
    inspect_thread(expected_native);

    pthread_t previous_native = fork_native_identity;
    native_identity = -ENOSYS;
    errno = EDOM;
    lume_pthread_fork_child();
    assert(errno == EDOM && identity == logical && fork_identity == logical &&
           fork_native_identity == previous_native && cleanup_stack == &record &&
           current_start == &start);
    assert(pthread_getspecific(inherited_key) == &inherited_value);
    __lume_cleanup_pop(&record, 0);
    current_start = NULL;
    assert(!pthread_key_delete(inherited_key));
    assert(!pthread_mutex_destroy(&mutex));

    identity = fork_identity = fork_native_identity = 0;
    errno = ERANGE;
    lume_pthread_fork_child();
    assert(errno == ERANGE && !identity && !fork_identity && !fork_native_identity);
    native_identity = UINT64_C(0x6606);
    lume_pthread_fork_child();
    assert(pthread_self() == (pthread_t)native_identity && fork_identity == identity &&
           fork_native_identity == identity);
    puts("PASS_PTHREAD_FORK_IDENTITY");
    return 0;
}
