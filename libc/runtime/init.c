#include "lume/init.h"
#include <stdbool.h>
#include <stdlib.h>

typedef void (*InitFunction)(void);
extern InitFunction __preinit_array_start[] __attribute__((weak));
extern InitFunction __preinit_array_end[] __attribute__((weak));
extern InitFunction __init_array_start[] __attribute__((weak));
extern InitFunction __init_array_end[] __attribute__((weak));
extern InitFunction __fini_array_start[] __attribute__((weak));
extern InitFunction __fini_array_end[] __attribute__((weak));
extern InitFunction __fini_array_onexit_start[] __attribute__((weak));
extern InitFunction __fini_array_onexit_end[] __attribute__((weak));

static bool constructed, destructed, c_initialized;
static bool libc_finalized, exit_callbacks_running;

static void forward(InitFunction *begin, InitFunction *end)
{
    if (begin && end)
        for (; begin != end; ++begin)
            (*begin)();
}

static void backward(InitFunction *begin, InitFunction *end)
{
    if (begin && end)
        while (end != begin)
            (*--end)();
}

void lume_runtime_construct(void)
{
    if (constructed)
        return;
    constructed = true;
    forward(__preinit_array_start, __preinit_array_end);
    forward(__init_array_start, __init_array_end);
}

void lume_runtime_destruct(void)
{
    if (destructed || exit_callbacks_running)
        return;
    destructed = true;
    backward(__fini_array_start, __fini_array_end);
}

void __libc_fini_array(void)
{
    if (libc_finalized)
        return;
    libc_finalized = true;
    /* Registered finalization callbacks finish before the destructor array. */
    exit_callbacks_running = true;
    backward(__fini_array_onexit_start, __fini_array_onexit_end);
    exit_callbacks_running = false;
    lume_runtime_destruct();
}

void lume_runtime_c_initialize(void)
{
    if (c_initialized)
        return;
    c_initialized = true;
    if (atexit(lume_runtime_destruct))
        abort();
    lume_runtime_construct();
}
