// SPDX-License-Identifier: BSD-3-Clause
// Itanium ABI hooks for a single-threaded Lume process, built without EH/RTTI.
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

extern "C"
{
    void *__dso_handle = &__dso_handle;

    int __cxa_guard_acquire(uint64_t *guard)
    {
        return !*(unsigned char *)guard;
    }

    void __cxa_guard_release(uint64_t *guard)
    {
        *(unsigned char *)guard = 1;
    }

    void __cxa_guard_abort(uint64_t *)
    {
    }

    [[noreturn]] __attribute__((weak)) void __cxa_pure_virtual()
    {
        fputs("pure virtual call\n", stderr);
        abort();
    }

    [[noreturn]] void __cxa_deleted_virtual()
    {
        fputs("deleted virtual call\n", stderr);
        abort();
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

    static CxxAtExit callbacks[256];
    static unsigned callback_count;
    static CxxAtExit *last_callback;
    static int exit_status;

    static CxxAtExit *new_callback()
    {
        CxxAtExit *entry;
        bool dynamic = callback_count == sizeof(callbacks) / sizeof(callbacks[0]);
        if (dynamic)
        {
            entry = static_cast<CxxAtExit *>(malloc(sizeof(CxxAtExit)));
            if (!entry)
                return nullptr;
        }
        else
            entry = &callbacks[callback_count++];
        *entry = {nullptr, nullptr, nullptr, nullptr, nullptr, last_callback, dynamic};
        last_callback = entry;
        return entry;
    }

    int __cxa_atexit(void (*f)(void *), void *arg, void *dso)
    {
        auto *entry = new_callback();
        if (!entry)
            return -1;
        entry->function = f;
        entry->argument = arg;
        entry->dso = dso;
        return 0;
    }

    int atexit(void (*function)())
    {
        auto *entry = new_callback();
        if (!entry)
            return -1;
        entry->cfunction = function;
        entry->dso = __dso_handle;
        return 0;
    }

    int on_exit(void (*function)(int, void *), void *argument)
    {
        auto *entry = new_callback();
        if (!entry)
            return -1;
        entry->onfunction = function;
        entry->argument = argument;
        return 0;
    }

    void __cxa_finalize(void *dso)
    {
        // Restart after each call: callbacks may register or finalize more callbacks.
        for (;;)
        {
            CxxAtExit *entry = last_callback;
            while (entry && (!(entry->function || entry->cfunction || entry->onfunction) ||
                             (dso && entry->dso != dso)))
                entry = entry->previous;
            if (!entry)
                break;
            auto function = entry->function;
            auto cfunction = entry->cfunction;
            auto onfunction = entry->onfunction;
            entry->function = nullptr;
            entry->cfunction = nullptr;
            entry->onfunction = nullptr;
            if (onfunction)
                onfunction(exit_status, entry->argument);
            else if (cfunction)
                cfunction();
            else
                function(entry->argument);
        }
        if (!dso)
            while (last_callback)
            {
                auto *old = last_callback;
                last_callback = old->previous;
                if (old->dynamic)
                    free(old);
            }
    }

    typedef void (*InitFunction)();
    extern InitFunction __init_array_start[], __init_array_end[];
    extern InitFunction __fini_array_start[], __fini_array_end[];
    void native_cxx_finalize();

    [[noreturn]] void exit(int status)
    {
        exit_status = status;
        native_cxx_finalize();
        _exit(status);
    }

    void native_cxx_initialize()
    {
        for (auto p = __init_array_start; p != __init_array_end; ++p)
            (*p)();
    }

    void native_cxx_finalize()
    {
        __cxa_finalize(nullptr);
        static bool finalized;
        if (!finalized)
        {
            finalized = true;
            for (auto p = __fini_array_end; p != __fini_array_start;)
                (*--p)();
        }
    }
}
