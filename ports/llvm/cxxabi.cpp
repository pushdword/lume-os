// SPDX-License-Identifier: BSD-3-Clause
#if !(defined(__aarch64__) || defined(__x86_64__) || defined(__i386__))
#include "../../libc/compat/legacy32/posix/cxxabi.cpp"
#else
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include "lume/init.h"

namespace
{
static pthread_mutex_t guard_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t guard_changed = PTHREAD_COND_INITIALIZER;
static thread_local unsigned char guard_identity;

[[noreturn]] void runtime_failure(const char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    abort();
}

void lock(pthread_mutex_t *mutex)
{
    if (pthread_mutex_lock(mutex))
        runtime_failure("C++ runtime mutex lock failed");
}

void unlock(pthread_mutex_t *mutex)
{
    if (pthread_mutex_unlock(mutex))
        runtime_failure("C++ runtime mutex unlock failed");
}

void wait(pthread_cond_t *condition, pthread_mutex_t *mutex)
{
    int error = pthread_cond_wait(condition, mutex);
    if (error && error != EINTR)
        runtime_failure("C++ runtime condition wait failed");
}

void notify(pthread_cond_t *condition)
{
    if (pthread_cond_broadcast(condition))
        runtime_failure("C++ runtime condition notification failed");
}

uint64_t initializer_identity()
{
    uint64_t value = (uintptr_t)&guard_identity;
    if (!value || value > (UINT64_MAX >> 8) || (unsigned char *)(uintptr_t)value != &guard_identity)
        runtime_failure("C++ initializer identity is not representable");
    return value;
}

uint64_t guard_owner(const unsigned char *guard)
{
    uint64_t owner = 0;
    for (unsigned i = 0; i < 7; ++i)
        owner |= (uint64_t)guard[i + 1] << (i * 8);
    return owner;
}

void guard_set_owner(unsigned char *guard, uint64_t owner)
{
    for (unsigned i = 0; i < 7; ++i)
        guard[i + 1] = (unsigned char)(owner >> (i * 8));
}

struct CxxAtExit
{
    void (*function)(void *);
    void (*cfunction)();
    void (*onfunction)(int, void *);
    void *argument;
    void *dso;
    CxxAtExit *previous;
    bool dynamic;
};

static pthread_mutex_t callback_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t callbacks_changed = PTHREAD_COND_INITIALIZER;
static CxxAtExit callbacks[256];
static unsigned callback_count;
static CxxAtExit *last_callback;
static unsigned active_callbacks;
static thread_local unsigned callback_depth;
static int exit_status;
static bool exit_started;
static unsigned finalize_state;
static uint64_t finalize_owner;

int register_callback(CxxAtExit record)
{
    CxxAtExit *entry;
    lock(&callback_mutex);
    if (finalize_state == 2)
    {
        unlock(&callback_mutex);
        errno = EBUSY;
        return -1;
    }
    if (callback_count < sizeof(callbacks) / sizeof(callbacks[0]))
    {
        entry = &callbacks[callback_count++];
        record.dynamic = false;
    }
    else
    {
        unlock(&callback_mutex);
        entry = static_cast<CxxAtExit *>(malloc(sizeof(*entry)));
        if (!entry)
            return -1;
        record.dynamic = true;
        lock(&callback_mutex);
        if (finalize_state == 2)
        {
            unlock(&callback_mutex);
            free(entry);
            errno = EBUSY;
            return -1;
        }
    }
    record.previous = last_callback;
    *entry = record;
    last_callback = entry;
    unlock(&callback_mutex);
    return 0;
}

struct CxxThreadAtExit
{
    void (*function)(void *);
    void *argument;
    void *dso;
    CxxThreadAtExit *previous;
};

static thread_local CxxThreadAtExit *thread_callbacks;
}

extern "C"
{
    void *__dso_handle = &__dso_handle;

    int __cxa_guard_acquire(uint64_t *storage)
    {
        auto *guard = reinterpret_cast<unsigned char *>(storage);
        if (__atomic_load_n(guard, __ATOMIC_ACQUIRE))
            return 0;
        uint64_t identity = initializer_identity();
        lock(&guard_mutex);
        for (;;)
        {
            if (__atomic_load_n(guard, __ATOMIC_ACQUIRE))
            {
                unlock(&guard_mutex);
                return 0;
            }
            uint64_t owner = guard_owner(guard);
            if (!owner)
            {
                // The initialized byte stays zero while the constructor runs.
                guard_set_owner(guard, identity);
                unlock(&guard_mutex);
                return 1;
            }
            if (owner == identity)
            {
                unlock(&guard_mutex);
                runtime_failure("Recursive C++ static initialization");
            }
            wait(&guard_changed, &guard_mutex);
        }
    }

    void __cxa_guard_release(uint64_t *storage)
    {
        auto *guard = reinterpret_cast<unsigned char *>(storage);
        uint64_t identity = initializer_identity();
        lock(&guard_mutex);
        if (__atomic_load_n(guard, __ATOMIC_RELAXED) || guard_owner(guard) != identity)
        {
            unlock(&guard_mutex);
            runtime_failure("Invalid C++ initializer release");
        }
        guard_set_owner(guard, 0);
        __atomic_store_n(guard, 1, __ATOMIC_RELEASE);
        notify(&guard_changed);
        unlock(&guard_mutex);
    }

    void __cxa_guard_abort(uint64_t *storage)
    {
        auto *guard = reinterpret_cast<unsigned char *>(storage);
        uint64_t identity = initializer_identity();
        lock(&guard_mutex);
        if (__atomic_load_n(guard, __ATOMIC_RELAXED) || guard_owner(guard) != identity)
        {
            unlock(&guard_mutex);
            runtime_failure("Invalid C++ initializer abort");
        }
        guard_set_owner(guard, 0);
        notify(&guard_changed);
        unlock(&guard_mutex);
    }

    [[noreturn]] __attribute__((weak)) void __cxa_pure_virtual()
    {
        runtime_failure("Pure virtual call");
    }

    [[noreturn]] void __cxa_deleted_virtual()
    {
        runtime_failure("Deleted virtual call");
    }

    int __cxa_atexit(void (*function)(void *), void *argument, void *dso)
    {
        return register_callback({function, nullptr, nullptr, argument, dso, nullptr, false});
    }

    int atexit(void (*function)())
    {
        return register_callback(
            {nullptr, function, nullptr, nullptr, __dso_handle, nullptr, false});
    }

    int on_exit(void (*function)(int, void *), void *argument)
    {
        return register_callback({nullptr, nullptr, function, argument, nullptr, nullptr, false});
    }

    void __cxa_finalize(void *dso)
    {
        for (;;)
        {
            lock(&callback_mutex);
            CxxAtExit **link = &last_callback;
            while (*link && dso && (*link)->dso != dso)
                link = &(*link)->previous;
            auto *entry = *link;
            if (!entry)
            {
                unlock(&callback_mutex);
                return;
            }
            *link = entry->previous;
            CxxAtExit record = *entry;
            ++active_callbacks;
            int status = exit_status;
            unlock(&callback_mutex);
            ++callback_depth;
            if (record.onfunction)
                record.onfunction(status, record.argument);
            else if (record.cfunction)
                record.cfunction();
            else if (record.function)
                record.function(record.argument);
            --callback_depth;
            if (record.dynamic)
                free(entry);
            lock(&callback_mutex);
            --active_callbacks;
            notify(&callbacks_changed);
            unlock(&callback_mutex);
        }
    }

    int __cxa_thread_atexit_impl(void (*function)(void *), void *argument, void *dso)
    {
        auto *entry = static_cast<CxxThreadAtExit *>(malloc(sizeof(CxxThreadAtExit)));
        if (!entry)
            return -1;
        *entry = {function, argument, dso, thread_callbacks};
        thread_callbacks = entry;
        return 0;
    }

    int __cxa_thread_atexit(void (*function)(void *), void *argument, void *dso)
    {
        return __cxa_thread_atexit_impl(function, argument, dso);
    }

    void native_cxx_thread_finalize()
    {
        while (thread_callbacks)
        {
            auto *entry = thread_callbacks;
            thread_callbacks = entry->previous;
            auto function = entry->function;
            void *argument = entry->argument;
            free(entry);
            function(argument);
        }
    }

    void native_cxx_finalize();

    [[noreturn]] void exit(int status)
    {
        lock(&callback_mutex);
        if (!exit_started)
        {
            exit_started = true;
            exit_status = status;
        }
        status = exit_status;
        unlock(&callback_mutex);
        native_cxx_finalize();
        _exit(status);
    }

    void native_cxx_initialize()
    {
        lume_runtime_construct();
    }

    void native_cxx_finalize()
    {
        uint64_t identity = initializer_identity();
        lock(&callback_mutex);
        while (finalize_state == 1 && finalize_owner != identity && !callback_depth)
            wait(&callbacks_changed, &callback_mutex);
        if (finalize_state)
        {
            unlock(&callback_mutex);
            return;
        }
        finalize_state = 1;
        finalize_owner = identity;
        unlock(&callback_mutex);
        native_cxx_thread_finalize();
        bool run_fini = true;
        for (;;)
        {
            __cxa_finalize(nullptr);
            lock(&callback_mutex);
            while (active_callbacks > callback_depth)
                wait(&callbacks_changed, &callback_mutex);
            if (last_callback)
            {
                unlock(&callback_mutex);
                continue;
            }
            if (!run_fini)
            {
                finalize_state = 2;
                notify(&callbacks_changed);
                unlock(&callback_mutex);
                return;
            }
            unlock(&callback_mutex);
            run_fini = false;
            lume_runtime_destruct();
        }
    }
}
#endif
