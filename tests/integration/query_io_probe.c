#include "lume/app_support.h"
#include "lume/thread.h"
#include "perf.h"
#include <stdbool.h>
#include <stdint.h>

#define READERS 4u
#define ITERATIONS 1024u
#define BLOCKS 64u
#define BLOCK_BYTES 4096u
static uint32_t ready, go, io_started, io_done;
static uint64_t pid;
static bool validate_errors = true;
volatile uint64_t query_io_progress[READERS];
static unsigned cores[READERS], overlaps[READERS], lines[READERS + 1];
static unsigned char buffer[BLOCK_BYTES];
static const char path[] = "/data/query-io-test";

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
    __atomic_store_n(&lines[id], line, __ATOMIC_RELEASE);
    struct lume_thread_request request = {.value = line};
    for (;;)
        (void)thread_call(LUME_THREAD_EXIT, &request);
}

static _Noreturn void run(uint64_t argument)
{
    unsigned id = (unsigned)argument;
    int64_t tid = call(LUME_SYS_THREAD, LUME_THREAD_SELF, 0, 0);
    if (tid <= 0)
        finish(id, __LINE__);
    __atomic_add_fetch(&ready, 1, __ATOMIC_RELEASE);
    while (!__atomic_load_n(&go, __ATOMIC_ACQUIRE))
    {
        struct lume_thread_request request = {.address = (uintptr_t)&go, .timeout_ms = 20000};
        int64_t result = thread_call(LUME_THREAD_WAIT, &request);
        if (result && result != -LUME_EAGAIN)
            finish(id, __LINE__);
    }
    if (id == READERS)
    {
        int64_t fd = open(path, LUME_O_RDWR | LUME_O_CREAT | LUME_O_TRUNC);
        if (fd < 0)
            finish(id, __LINE__);
        for (unsigned i = 0; i < BLOCK_BYTES; ++i)
            buffer[i] = (unsigned char)(i * 17u + (i >> 4));
        __atomic_store_n(&io_started, 1, __ATOMIC_RELEASE);
        for (unsigned block = 0; block < BLOCKS; ++block)
            if (write_all((int)fd, buffer, sizeof(buffer)) != sizeof(buffer))
                finish(id, __LINE__);
        if (call(LUME_SYS_SEEK, (uint64_t)fd, 0, 0))
            finish(id, __LINE__);
        for (unsigned block = 0; block < BLOCKS; ++block)
        {
            if (read((int)fd, buffer, sizeof(buffer)) != sizeof(buffer))
                finish(id, __LINE__);
            for (unsigned i = 0; i < BLOCK_BYTES; ++i)
                if (buffer[i] != (unsigned char)(i * 17u + (i >> 4)))
                    finish(id, __LINE__);
        }
        if (close((int)fd))
            finish(id, __LINE__);
        __atomic_store_n(&io_done, 1, __ATOMIC_RELEASE);
        finish(id, 0);
    }
    uint64_t previous = (uint64_t)call(LUME_SYS_CLOCK, 0, 0, 0);
    uint64_t deadline = previous + 30000;
    while (!__atomic_load_n(&io_started, __ATOMIC_ACQUIRE))
        if ((uint64_t)call(LUME_SYS_CLOCK, 0, 0, 0) >= deadline || call(LUME_SYS_SLEEP, 1, 0, 0))
            finish(id, __LINE__);
    for (unsigned i = 0; i < ITERATIONS; ++i)
    {
        int64_t at = call(LUME_SYS_CLOCK, 0, 0, 0);
        int64_t cpu = call(LUME_SYS_CPU, 1, 0, 0);
        if (at < 0 || (uint64_t)at < previous || (uint64_t)at >= deadline || cpu < 0 || cpu >= 4 ||
            call(LUME_SYS_ABI, 0, 0, 0) != LUME_ABI_VERSION ||
            call(LUME_SYS_GETPID, 0, 0, 0) != (int64_t)pid ||
            call(LUME_SYS_THREAD, LUME_THREAD_SELF, 0, 0) != tid ||
            (validate_errors && call(LUME_SYS_CPU, 3, 0, 0) != -LUME_EINVAL))
            finish(id, __LINE__);
        previous = (uint64_t)at;
        cores[id] |= 1u << (unsigned)cpu;
        if (!__atomic_load_n(&io_done, __ATOMIC_ACQUIRE))
            ++overlaps[id];
        __atomic_store_n(&query_io_progress[id], i + 1, __ATOMIC_RELEASE);
    }
    finish(id, 0);
}

int main(int argc, char **argv)
{
    if (argc == 2 && eq(argv[1], "benchmark"))
        validate_errors = false;
    else if (argc != 1)
        return 9;
    struct lume_perf_snapshot before, after;
    if (call(LUME_SYS_CPU, 0, 0, 0) != 4 ||
        call(LUME_SYS_TELEMETRY, (uintptr_t)&before, sizeof(before), LUME_PERF_VERSION))
        return 1;
    pid = (uint64_t)call(LUME_SYS_GETPID, 0, 0, 0);
    uint64_t tids[READERS + 1];
    for (unsigned i = 0; i <= READERS; ++i)
    {
        struct lume_thread_request request = {
            .entry = (uintptr_t)run, .argument = i, .stack_bytes = 32768};
        int64_t tid = thread_call(LUME_THREAD_CREATE, &request);
        if (tid <= 0)
            return 2;
        tids[i] = (uint64_t)tid;
    }
    uint64_t started = (uint64_t)call(LUME_SYS_CLOCK, 0, 0, 0), deadline = started + 30000;
    while (__atomic_load_n(&ready, __ATOMIC_ACQUIRE) != READERS + 1)
        if ((uint64_t)call(LUME_SYS_CLOCK, 0, 0, 0) >= deadline || call(LUME_SYS_SLEEP, 1, 0, 0))
            return 3;
    __atomic_store_n(&go, 1, __ATOMIC_RELEASE);
    struct lume_thread_request wake = {.address = (uintptr_t)&go, .value = READERS + 1};
    if (thread_call(LUME_THREAD_WAKE, &wake) < 0)
        return 4;
    bool observed = false;
    while (!observed)
    {
        observed = true;
        for (unsigned i = 0; i < READERS; ++i)
            observed = observed && __atomic_load_n(&query_io_progress[i], __ATOMIC_ACQUIRE) >= 64;
        if ((uint64_t)call(LUME_SYS_CLOCK, 0, 0, 0) >= deadline || call(LUME_SYS_SLEEP, 1, 0, 0))
        {
            puts("QUERY_IO_TIMEOUT started=");
            number(__atomic_load_n(&io_started, __ATOMIC_ACQUIRE));
            puts(" done=");
            number(__atomic_load_n(&io_done, __ATOMIC_ACQUIRE));
            for (unsigned i = 0; i <= READERS; ++i)
            {
                puts(" line=");
                number(__atomic_load_n(&lines[i], __ATOMIC_ACQUIRE));
                if (i < READERS)
                {
                    puts(" progress=");
                    number(__atomic_load_n(&query_io_progress[i], __ATOMIC_ACQUIRE));
                }
            }
            puts("\n");
            return 5;
        }
    }
    puts("QUERY_IO_READERS_PROGRESS\n");
    unsigned mask = 0, overlap = 0;
    for (unsigned i = 0; i <= READERS; ++i)
    {
        struct lume_thread_request request = {.tid = tids[i]};
        if (thread_call(LUME_THREAD_JOIN, &request) || request.value)
        {
            puts("QUERY_IO_FAIL thread=");
            number(i);
            puts(" line=");
            number(request.value);
            puts("\n");
            return 6;
        }
        if (i < READERS)
        {
            if (__atomic_load_n(&query_io_progress[i], __ATOMIC_ACQUIRE) != ITERATIONS)
                return 7;
            mask |= cores[i];
            overlap += overlaps[i];
        }
    }
    if (!overlap || !__atomic_load_n(&io_done, __ATOMIC_ACQUIRE) ||
        call(LUME_SYS_TELEMETRY, (uintptr_t)&after, sizeof(after), LUME_PERF_VERSION))
        return 8;
    uint64_t locked = 0, kernel = 0;
    for (unsigned i = 0; i < 4; ++i)
    {
        locked += (uint32_t)(after.cores[i].ticks[LUME_PERF_LOCKWAIT] -
                             before.cores[i].ticks[LUME_PERF_LOCKWAIT]);
        kernel += (uint32_t)(after.cores[i].ticks[LUME_PERF_KERNEL] -
                             before.cores[i].ticks[LUME_PERF_KERNEL]);
    }
    puts("QUERY_IO_PASS queries=");
    number(READERS * ITERATIONS * (validate_errors ? 6u : 5u));
    puts(" bytes=262144 cores=");
    number(mask);
    puts(" overlap=");
    number(overlap);
    puts(" elapsed_ms=");
    number((uint64_t)call(LUME_SYS_CLOCK, 0, 0, 0) - started);
    puts(" lock_ticks=");
    number(locked);
    puts(" kernel_ticks=");
    number(kernel);
    puts(" clock_hz=");
    number(before.clock_hz);
    puts("\n");
    return 0;
}
