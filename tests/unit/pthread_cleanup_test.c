#define _POSIX_C_SOURCE 200809L
#define _WIN32_WINNT 0x0602
#include <assert.h>
#include <errno.h>
#include <process.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "lume/thread.h"
#include "../fixtures/pthread_host_namespace.h"

static unsigned allocations, releases;
static _Thread_local int allocation_budget = -1;

static void *product_malloc(size_t bytes)
{
    if (!allocation_budget)
        return NULL;
    if (allocation_budget > 0)
        --allocation_budget;
    void *result = malloc(bytes);
    if (result)
        __atomic_add_fetch(&allocations, 1, __ATOMIC_RELAXED);
    return result;
}

static void product_free(void *pointer)
{
    if (pointer)
        __atomic_add_fetch(&releases, 1, __ATOMIC_RELAXED);
    free(pointer);
}

#ifndef LUME_TEST_PTHREAD_SOURCE
#define LUME_TEST_PTHREAD_SOURCE "../../libc/posix/pthread.c"
#endif
#define malloc product_malloc
#define free product_free
#include LUME_TEST_PTHREAD_SOURCE
#undef malloc
#undef free

static CRITICAL_SECTION scheduler;
static _Thread_local uint64_t host_tid = 1;
static _Thread_local struct host_thread *running;
static uint64_t next_tid = 1;

static struct host_thread
{
    HANDLE handle;
    uint64_t tid, entry, argument, value;
} host_threads[16];

static unsigned __stdcall host_entry(void *argument)
{
    running = argument;
    host_tid = running->tid;
    ((void (*)(void *))(uintptr_t)running->entry)((void *)(uintptr_t)running->argument);
    assert(!"Thread trampoline returned");
    return 1;
}

int64_t lume_thread_call(unsigned operation, struct lume_thread_request *request)
{
    if (operation == LUME_THREAD_SELF)
        return (int64_t)host_tid;
    if (operation == LUME_THREAD_EXIT)
    {
        assert(running);
        running->value = request->value;
        _endthreadex(0);
    }
    if (operation == LUME_THREAD_WAIT)
    {
        uint32_t *word = (uint32_t *)(uintptr_t)request->address;
        ULONGLONG deadline = GetTickCount64() + 20000;
        while (__atomic_load_n(word, __ATOMIC_ACQUIRE) == (uint32_t)request->value)
        {
            assert(GetTickCount64() < deadline);
            Sleep(0);
        }
        return 0;
    }
    if (operation == LUME_THREAD_WAKE)
        return 0;
    if (operation == LUME_THREAD_CREATE)
    {
        EnterCriticalSection(&scheduler);
        struct host_thread *thread = NULL;
        for (unsigned i = 0; i < sizeof(host_threads) / sizeof(host_threads[0]); ++i)
            if (!host_threads[i].tid)
            {
                thread = &host_threads[i];
                break;
            }
        assert(thread);
        *thread = (struct host_thread){
            .tid = ++next_tid, .entry = request->entry, .argument = request->argument};
        thread->handle = (HANDLE)_beginthreadex(NULL, 0, host_entry, thread, 0, NULL);
        assert(thread->handle);
        uint64_t tid = thread->tid;
        LeaveCriticalSection(&scheduler);
        return (int64_t)tid;
    }
    if (operation == LUME_THREAD_JOIN)
    {
        EnterCriticalSection(&scheduler);
        struct host_thread *thread = NULL;
        for (unsigned i = 0; i < sizeof(host_threads) / sizeof(host_threads[0]); ++i)
            if (host_threads[i].tid == request->tid)
            {
                thread = &host_threads[i];
                break;
            }
        assert(thread);
        HANDLE handle = thread->handle;
        LeaveCriticalSection(&scheduler);
        assert(WaitForSingleObject(handle, 30000) == WAIT_OBJECT_0);
        request->value = thread->value;
        assert(CloseHandle(handle));
        EnterCriticalSection(&scheduler);
        *thread = (struct host_thread){0};
        LeaveCriticalSection(&scheduler);
        return 0;
    }
    assert(!"Unexpected thread operation");
    return -ENOSYS;
}

#define main cleanup_probe_main
#include "../integration/pthread_cleanup_probe.c"
#undef main

#ifndef LUME_PTHREAD_HOST_HARNESS_ONLY
int main(void)
{
    InitializeCriticalSection(&scheduler);
    unsigned before = __atomic_load_n(&allocations, __ATOMIC_RELAXED);
    struct trace trace = {0};
    current = &trace;
    struct action action = {&trace, 5};
    pthread_cleanup_push(reenter, &action);
    pthread_cleanup_pop(1);
    assert(__atomic_load_n(&allocations, __ATOMIC_RELAXED) == before);
    assert(cleanup_probe_main() == 0);
    assert(cleanup_stack == NULL);
#ifdef KEY_SEGMENT_SIZE
    unsigned retained = 0;
    for (unsigned group = 0; group < KEY_SEGMENT_SIZE; ++group)
    {
        assert(key_values[group] == NULL);
        if (key_segments[group])
        {
            ++retained;
            for (unsigned i = 0; i < KEY_SEGMENT_SIZE; ++i)
                assert(!__atomic_load_n(&key_segments[group]->entries[i].active, __ATOMIC_ACQUIRE));
        }
    }
    assert(allocations == releases + retained && starts == NULL);
#else
    assert(allocations == releases);
    assert(starts == NULL && keys == NULL && all_values == NULL);
#endif
    for (unsigned i = 0; i < sizeof(host_threads) / sizeof(host_threads[0]); ++i)
        assert(!host_threads[i].tid);
    DeleteCriticalSection(&scheduler);
    puts("pthread cleanup host PASS: actual TLS threads, released values and bounded key metadata");
    return 0;
}
#endif
