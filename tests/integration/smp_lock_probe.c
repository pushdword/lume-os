#include "lume/app_support.h"
#include "lume/thread.h"
#include "perf.h"
#include <stdbool.h>
#include <stdint.h>

#define THREADS 8u
#define ROUNDS 2u
#define ITERATIONS 2048u
#define CANARY UINT64_C(0x374c4f434b534d50)

static uint32_t ready, go;
static uint64_t pid;

static struct
{
    uint64_t tid, progress, hash;
    unsigned cores, line;
} results[THREADS];

static void number(uint64_t value)
{
    char text[21];
    unsigned at = sizeof(text) - 1;
    text[at] = 0;
    do
    {
        text[--at] = (char)('0' + value % 10);
        value /= 10;
    } while (value);
    puts(text + at);
}

static int64_t thread_call(unsigned operation, struct lume_thread_request *request)
{
    return call(LUME_SYS_THREAD, operation, (uintptr_t)request, sizeof(*request));
}

static _Noreturn void finish(unsigned id, unsigned line)
{
    results[id].line = line;
    struct lume_thread_request request = {.value = line};
    for (;;)
        (void)thread_call(LUME_THREAD_EXIT, &request);
}

static _Noreturn void run(uint64_t argument)
{
    unsigned id = (unsigned)argument;
    volatile uint64_t canary[32];
    for (unsigned i = 0; i < 32; ++i)
        canary[i] = CANARY ^ (id * 32u + i);
    int64_t tid = call(LUME_SYS_THREAD, LUME_THREAD_SELF, 0, 0);
    if (tid <= 0)
        finish(id, __LINE__);
    results[id].tid = (uint64_t)tid;
    __atomic_add_fetch(&ready, 1, __ATOMIC_RELEASE);
    while (!__atomic_load_n(&go, __ATOMIC_ACQUIRE))
    {
        struct lume_thread_request request = {.address = (uintptr_t)&go, .timeout_ms = 20000};
        int64_t result = thread_call(LUME_THREAD_WAIT, &request);
        if (result && result != -LUME_EAGAIN)
            finish(id, __LINE__);
    }
    uint64_t previous = (uint64_t)call(LUME_SYS_CLOCK, 0, 0, 0), started = previous;
    uint64_t hash = 0;
    for (unsigned i = 0; i < ITERATIONS; ++i)
    {
        int64_t at = call(LUME_SYS_CLOCK, 0, 0, 0);
        int64_t cpu = call(LUME_SYS_CPU, 1, 0, 0);
        if (at < 0 || (uint64_t)at < previous || (uint64_t)at - started > 20000 || cpu < 0 ||
            cpu >= 4 || call(LUME_SYS_ABI, 0, 0, 0) != LUME_ABI_VERSION ||
            call(LUME_SYS_GETPID, 0, 0, 0) != (int64_t)pid ||
            call(LUME_SYS_THREAD, LUME_THREAD_SELF, 0, 0) != tid)
            finish(id, __LINE__);
        previous = (uint64_t)at;
        results[id].cores |= 1u << (unsigned)cpu;
        hash = (hash * 33u) ^ (id * ITERATIONS + i);
        for (unsigned j = 0; j < 32; ++j)
            if (canary[j] != (CANARY ^ (id * 32u + j)))
                finish(id, __LINE__);
        __atomic_store_n(&results[id].progress, i + 1, __ATOMIC_RELEASE);
        if (!(i & 127u) && call(LUME_SYS_SLEEP, 0, 0, 0))
            finish(id, __LINE__);
    }
    results[id].hash = hash;
    finish(id, 0);
}

int main(void)
{
    struct lume_perf_snapshot before, after;
    if (call(LUME_SYS_CPU, 0, 0, 0) != 4 ||
        call(LUME_SYS_TELEMETRY, (uintptr_t)&before, sizeof(before), LUME_PERF_VERSION))
        return 1;
    pid = (uint64_t)call(LUME_SYS_GETPID, 0, 0, 0);
    unsigned cores = 0;
    for (unsigned round = 0; round < ROUNDS; ++round)
    {
        uint64_t tids[THREADS];
        __atomic_store_n(&go, 0, __ATOMIC_RELAXED);
        __atomic_store_n(&ready, 0, __ATOMIC_RELAXED);
        for (unsigned i = 0; i < THREADS; ++i)
        {
            results[i].tid = results[i].progress = results[i].hash = 0;
            results[i].cores = results[i].line = 0;
            struct lume_thread_request request = {
                .entry = (uintptr_t)run, .argument = i, .stack_bytes = 32768};
            int64_t tid = thread_call(LUME_THREAD_CREATE, &request);
            if (tid <= 0)
                return 2;
            tids[i] = (uint64_t)tid;
        }
        uint64_t deadline = (uint64_t)call(LUME_SYS_CLOCK, 0, 0, 0) + 20000;
        while (__atomic_load_n(&ready, __ATOMIC_ACQUIRE) != THREADS)
            if ((uint64_t)call(LUME_SYS_CLOCK, 0, 0, 0) >= deadline ||
                call(LUME_SYS_SLEEP, 1, 0, 0))
                return 3;
        __atomic_store_n(&go, 1, __ATOMIC_RELEASE);
        struct lume_thread_request wake = {.address = (uintptr_t)&go, .value = THREADS};
        if (thread_call(LUME_THREAD_WAKE, &wake) < 0)
            return 4;
        for (unsigned i = 0; i < THREADS; ++i)
        {
            struct lume_thread_request request = {.tid = tids[i]};
            if (thread_call(LUME_THREAD_JOIN, &request) || request.value || results[i].line ||
                results[i].tid != tids[i] ||
                __atomic_load_n(&results[i].progress, __ATOMIC_ACQUIRE) != ITERATIONS)
            {
                puts("SMP_LOCK_FAIL thread=");
                number(i);
                puts(" line=");
                number(results[i].line);
                puts("\n");
                return 5;
            }
            uint64_t hash = 0;
            for (unsigned j = 0; j < ITERATIONS; ++j)
                hash = (hash * 33u) ^ (i * ITERATIONS + j);
            if (results[i].hash != hash)
                return 6;
            cores |= results[i].cores;
        }
    }
    if (cores != 15 ||
        call(LUME_SYS_TELEMETRY, (uintptr_t)&after, sizeof(after), LUME_PERF_VERSION))
        return 7;
    for (unsigned i = 0; i < 4; ++i)
    {
        uint32_t calls = after.cores[i].syscalls - before.cores[i].syscalls;
        if (!calls)
            return 8;
        puts("SMP_LOCK_CPU=");
        number(i);
        puts(" calls=");
        number(calls);
        puts(" lock_ticks=");
        number(after.cores[i].ticks[LUME_PERF_LOCKWAIT] -
               before.cores[i].ticks[LUME_PERF_LOCKWAIT]);
        puts("\n");
    }
    puts("SMP_LOCK_PASS threads=8 rounds=2 iterations=32768 cores=15\n");
    return 0;
}
