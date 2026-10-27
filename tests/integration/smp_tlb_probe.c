#include "lume/app_support.h"
#include "lume/thread.h"
#include <stdbool.h>
#include <stdint.h>

#define READERS 3u
#define ROUNDS 64u
static unsigned phase, primed[READERS], completed[READERS], failure, holder_stop;
static volatile uint64_t *watched;

static uint64_t pattern(unsigned round)
{
    return UINT64_C(0x7e4c425450000000) ^ (UINT64_C(0x9e3779b97f4a7c15) * round);
}

static _Noreturn void finish(unsigned status)
{
    struct lume_thread_request request = {.value = status};
    for (;;)
        (void)call(LUME_SYS_THREAD, LUME_THREAD_EXIT, (uintptr_t)&request, sizeof(request));
}

static _Noreturn void reader(uint64_t argument)
{
    unsigned id = (unsigned)argument;
    for (unsigned round = 1; round <= ROUNDS; ++round)
    {
        while (__atomic_load_n(&phase, __ATOMIC_ACQUIRE) != round * 2 - 1)
        {
        }
        if (*watched != pattern(round - 1))
        {
            __atomic_store_n(&failure, __LINE__, __ATOMIC_RELEASE);
            finish(1);
        }
        __atomic_store_n(&primed[id], round, __ATOMIC_RELEASE);
        while (__atomic_load_n(&phase, __ATOMIC_ACQUIRE) != round * 2)
        {
        }
        if (*watched != pattern(round))
        {
            __atomic_store_n(&failure, __LINE__, __ATOMIC_RELEASE);
            finish(2);
        }
        __atomic_store_n(&completed[id], round, __ATOMIC_RELEASE);
    }
    finish(0);
}

static _Noreturn void holder(uint64_t round)
{
    while (__atomic_load_n(&holder_stop, __ATOMIC_ACQUIRE) != round)
    {
    }
    finish(0);
}

static int64_t create(void (*entry)(uint64_t), uint64_t argument)
{
    struct lume_thread_request request = {
        .entry = (uintptr_t)entry, .argument = argument, .stack_bytes = 32768};
    return call(LUME_SYS_THREAD, LUME_THREAD_CREATE, (uintptr_t)&request, sizeof(request));
}

static bool join(uint64_t tid)
{
    struct lume_thread_request request = {.tid = tid};
    return !call(LUME_SYS_THREAD, LUME_THREAD_JOIN, (uintptr_t)&request, sizeof(request)) &&
           !request.value;
}

static bool wait_readers(unsigned *values, unsigned round)
{
    uint64_t deadline = (uint64_t)call(LUME_SYS_CLOCK, 0, 0, 0) + 10000;
    for (unsigned id = 0; id < READERS; ++id)
        while (__atomic_load_n(&values[id], __ATOMIC_ACQUIRE) != round)
        {
            if (__atomic_load_n(&failure, __ATOMIC_ACQUIRE) ||
                (uint64_t)call(LUME_SYS_CLOCK, 0, 0, 0) > deadline || call(LUME_SYS_SLEEP, 0, 0, 0))
                return false;
        }
    return true;
}

int main(void)
{
    if (call(LUME_SYS_CPU, 0, 0, 0) != 4 || call(LUME_SYS_PROBE, 0x8000, 1, 0) != -LUME_EFAULT)
        return 1;
    int64_t base = call(LUME_SYS_BRK, 0, 0, 0);
    if (base <= 0 || (base & 4095) ||
        call(LUME_SYS_BRK, (uint64_t)base + 4096, 0, 0) != base + 4096)
        return 2;
    watched = (void *)(uintptr_t)base;
    *watched = pattern(0);
    uint64_t tids[READERS];
    for (unsigned id = 0; id < READERS; ++id)
    {
        int64_t tid = create(reader, id);
        if (tid <= 0)
            return 3;
        tids[id] = (uint64_t)tid;
    }
    for (unsigned round = 1; round <= ROUNDS; ++round)
    {
        __atomic_store_n(&phase, round * 2 - 1, __ATOMIC_RELEASE);
        if (!wait_readers(primed, round) || call(LUME_SYS_BRK, (uint64_t)base, 0, 0) != base)
            return 4;
        /* Keep thread storage allocated while the heap address receives a new mapping. */
        int64_t tid = create(holder, round);
        if (tid <= 0 || call(LUME_SYS_BRK, (uint64_t)base + 4096, 0, 0) != base + 4096)
            return 5;
        *watched = pattern(round);
        __atomic_store_n(&phase, round * 2, __ATOMIC_RELEASE);
        if (!wait_readers(completed, round))
        {
            puts("SMP_TLB_FAIL stale translation or reader timeout\n");
            return 6;
        }
        __atomic_store_n(&holder_stop, round, __ATOMIC_RELEASE);
        if (!join((uint64_t)tid))
            return 7;
    }
    for (unsigned id = 0; id < READERS; ++id)
        if (!join(tids[id]))
            return 8;
    if (call(LUME_SYS_BRK, (uint64_t)base, 0, 0) != base)
        return 9;
    puts("SMP_TLB_PASS rounds=64 readers=3 low-memory-inaccessible\n");
    return 0;
}
