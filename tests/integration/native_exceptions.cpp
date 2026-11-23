#include <exception>
#include <pthread.h>
#include <stdexcept>
#include <stdio.h>
#include <string.h>

namespace
{
struct UnwindProbe
{
    int *destroyed;
    bool *unwinding;

    ~UnwindProbe()
    {
        ++*destroyed;
        *unwinding = std::uncaught_exceptions() > 0;
    }
};

[[noreturn]] void throw_nested(int *destroyed, bool *unwinding)
{
    UnwindProbe probe{destroyed, unwinding};
    throw std::runtime_error("native exception");
}

void rethrow_nested(int *destroyed, bool *unwinding)
{
    UnwindProbe probe{destroyed, unwinding};
    try
    {
        throw_nested(destroyed, unwinding);
    }
    catch (const std::runtime_error &error)
    {
        if (strcmp(error.what(), "native exception"))
            throw std::logic_error("exception payload changed");
        throw;
    }
}

void *thread_main(void *argument)
{
    auto *caught = static_cast<unsigned *>(argument);
    for (unsigned value = 1; value <= 64; ++value)
    {
        try
        {
            throw value;
        }
        catch (unsigned actual)
        {
            if (actual != value)
                return reinterpret_cast<void *>(1);
            ++*caught;
        }
    }
    return nullptr;
}
}

int main()
{
    int destroyed = 0;
    bool unwinding = false;
    std::exception_ptr saved;
    try
    {
        rethrow_nested(&destroyed, &unwinding);
    }
    catch (const std::exception &error)
    {
        if (strcmp(error.what(), "native exception"))
            return 1;
        saved = std::current_exception();
    }
    if (destroyed != 2 || !unwinding || !saved)
        return 2;
    try
    {
        std::rethrow_exception(saved);
    }
    catch (const std::runtime_error &error)
    {
        if (strcmp(error.what(), "native exception"))
            return 3;
    }
    unsigned caught = 0;
    pthread_t thread;
    if (pthread_create(&thread, nullptr, thread_main, &caught))
        return 4;
    void *result = nullptr;
    if (pthread_join(thread, &result) || result || caught != 64)
        return 5;
    puts("NATIVE_EXCEPTIONS_UNWIND_TLS_PASS");
    return 0;
}
