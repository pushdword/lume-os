#include "lume/app_support.h"
#include "lume/thread.h"
#include "lume/signal.h"
#include "fd.h"
#include "perf.h"
#include <stdbool.h>
#include <stdint.h>

#define THREADS 4u
#define BLOCKS 128u
uint64_t console_split_progress[THREADS];
struct lume_perf_snapshot console_split_perf;
uint64_t console_split_perf_samples;
static uint32_t ready, go, stop;
static uint64_t pid;

static int wait_input(char wanted)
{
    char byte = 0;
    while (byte != wanted)
    {
        int64_t result = read(0, &byte, 1);
        if (result == -LUME_EAGAIN)
        {
            if (call(LUME_SYS_SLEEP, 1, 0, 0))
                return 1;
        }
        else if (result != 1)
            return 1;
    }
    return 0;
}

static int64_t thread_call(unsigned operation, struct lume_thread_request *request)
{
    return call(LUME_SYS_THREAD, operation, (uintptr_t)request, sizeof(*request));
}

static _Noreturn void finish(unsigned result)
{
    struct lume_thread_request request = {.value = result};
    for (;;)
        (void)thread_call(LUME_THREAD_EXIT, &request);
}

static _Noreturn void observer(uint64_t id)
{
    __atomic_add_fetch(&ready, 1, __ATOMIC_RELEASE);
    uint64_t previous = 0;
    while (!__atomic_load_n(&stop, __ATOMIC_ACQUIRE))
    {
        int64_t at = call(LUME_SYS_CLOCK, 0, 0, 0);
        if (at < 0 || (uint64_t)at < previous || call(LUME_SYS_GETPID, 0, 0, 0) != (int64_t)pid)
            finish(__LINE__);
        previous = (uint64_t)at;
        __atomic_add_fetch(&console_split_progress[id], 1, __ATOMIC_RELEASE);
        if (!id && !(console_split_progress[0] & 63u))
        {
            if (call(LUME_SYS_TELEMETRY, (uintptr_t)&console_split_perf, sizeof(console_split_perf),
                     LUME_PERF_VERSION))
                finish(__LINE__);
            __atomic_add_fetch(&console_split_perf_samples, 1, __ATOMIC_RELEASE);
        }
    }
    finish(0);
}

static _Noreturn void writer(uint64_t id)
{
    __atomic_add_fetch(&ready, 1, __ATOMIC_RELEASE);
    while (!__atomic_load_n(&go, __ATOMIC_ACQUIRE))
        if (call(LUME_SYS_SLEEP, 1, 0, 0))
            finish(__LINE__);
    for (unsigned block = 0; block < BLOCKS; ++block)
    {
        struct
        {
            uint64_t before;
            char bytes[64];
            uint64_t after;
        } payload = {.before = UINT64_C(0x1234abcdef987654), .after = UINT64_C(0xfedc123456789abc)};

        const char prefix[] = "CONSOLE_RECORD ";
        for (unsigned i = 0; i < sizeof(prefix) - 1; ++i)
            payload.bytes[i] = prefix[i];
        payload.bytes[15] = (char)('0' + id);
        payload.bytes[16] = ':';
        for (unsigned i = 0; i < 4; ++i)
            payload.bytes[17 + i] = "0123456789abcdef"[(block >> ((3 - i) * 4)) & 15];
        payload.bytes[21] = '|';
        for (unsigned i = 22; i < 63; ++i)
            payload.bytes[i] = (char)('a' + ((id * 7u + block + i) % 26));
        payload.bytes[63] = '\n';
        if (write(1, payload.bytes, sizeof(payload.bytes)) != sizeof(payload.bytes) ||
            payload.before != UINT64_C(0x1234abcdef987654) ||
            payload.after != UINT64_C(0xfedc123456789abc))
            finish(__LINE__);
    }
    finish(0);
}

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

int main(int argc, char **argv)
{
    bool stress = argc == 2 && eq(argv[1], "stress");
    bool exiting = argc == 2 && eq(argv[1], "exit");
    if (call(LUME_SYS_CPU, 0, 0, 0) != 4)
        return 1;
    pid = (uint64_t)call(LUME_SYS_GETPID, 0, 0, 0);
    int start_pipe[2] = {-1, -1};
    int64_t child = 0;
    if (exiting)
    {
        struct lume_fd_request pipe = {.version = LUME_FD_VERSION, .bytes = sizeof(pipe)};
        if (call(LUME_SYS_FD, LUME_FD_PIPE, (uintptr_t)&pipe, sizeof(pipe)))
            return 8;
        start_pipe[0] = pipe.fd;
        start_pipe[1] = pipe.other;
        child = call(LUME_SYS_FORK, 0, 0, 0);
        if (child < 0)
            return 9;
        if (!child)
        {
            if (close(start_pipe[1]))
                return 10;
            char byte;
            if (read(start_pipe[0], &byte, 1) != 1 || byte != 'T')
                return 11;
            static const char payload[] =
                "CONSOLE_SPLIT_TX_PAYLOAD|abcdefghijklmnopqrstuvwxyz|END\n";
            (void)write(1, payload, sizeof(payload) - 1);
            return 88;
        }
        if (close(start_pipe[0]))
            return 12;
    }
    uint64_t tids[THREADS];
    for (unsigned i = 0; i < THREADS; ++i)
    {
        struct lume_thread_request request = {
            .entry = (uintptr_t)(stress ? writer : observer), .argument = i, .stack_bytes = 32768};
        int64_t tid = thread_call(LUME_THREAD_CREATE, &request);
        if (tid <= 0)
            return 2;
        tids[i] = (uint64_t)tid;
    }
    uint64_t deadline = (uint64_t)call(LUME_SYS_CLOCK, 0, 0, 0) + 10000;
    while (__atomic_load_n(&ready, __ATOMIC_ACQUIRE) != THREADS)
        if ((uint64_t)call(LUME_SYS_CLOCK, 0, 0, 0) > deadline || call(LUME_SYS_SLEEP, 1, 0, 0))
            return 3;
    if (stress)
        __atomic_store_n(&go, 1, __ATOMIC_RELEASE);
    else
    {
        puts("CONSOLE_SPLIT_READY pid=");
        number(pid);
        if (exiting)
        {
            puts(" writer=");
            number((uint64_t)child);
        }
        puts("\n");
        if (wait_input('T'))
            return 4;
        if (exiting)
        {
            if (write(start_pipe[1], "T", 1) != 1 || wait_input('K'))
                return 13;
            struct lume_signal_request request = {.version = LUME_SIGNAL_VERSION,
                                                  .bytes = sizeof(request),
                                                  .pid = (int32_t)child,
                                                  .number = 9};
            if (call(LUME_SYS_SIGNAL, LUME_SIGNAL_SEND, (uintptr_t)&request, sizeof(request)))
                return 14;
            int64_t status = call(LUME_SYS_WAIT, (uint64_t)child, 1, 0);
            if (status < 0 || ((uint64_t)status >> 32) != (uint64_t)child ||
                (uint32_t)status != 137 || close(start_pipe[1]))
                return 15;
        }
        else
        {
            static const char payload[] =
                "CONSOLE_SPLIT_TX_PAYLOAD|abcdefghijklmnopqrstuvwxyz|END\n";
            if (write(1, payload, sizeof(payload) - 1) != sizeof(payload) - 1)
                return 6;
        }
        __atomic_store_n(&stop, 1, __ATOMIC_RELEASE);
    }
    for (unsigned i = 0; i < THREADS; ++i)
    {
        struct lume_thread_request request = {.tid = tids[i]};
        if (thread_call(LUME_THREAD_JOIN, &request) || request.value)
            return 7;
    }
    puts(stress    ? "CONSOLE_SPLIT_STRESS_PASS records=512 bytes=32768\n"
         : exiting ? "CONSOLE_SPLIT_EXIT_PASS status=137\n"
                   : "CONSOLE_SPLIT_HOLD_PASS\n");
    return 0;
}
