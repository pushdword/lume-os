#define _GNU_SOURCE
#define lume_stat lume_native_stat
#define LUME_SEEK_SET LUME_NATIVE_SEEK_SET
#define LUME_SEEK_CUR LUME_NATIVE_SEEK_CUR
#define LUME_SEEK_END LUME_NATIVE_SEEK_END
#include "lume/abi.h"
#undef lume_stat
#undef LUME_SEEK_SET
#undef LUME_SEEK_CUR
#undef LUME_SEEK_END
#undef LUME_ARG_BYTES
#include "api.h"
#include "fd.h"
#include <assert.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

static uint64_t size = 12;
static int64_t position;
static uint32_t kind = LUME_NODE_FILE;
static int stat_error;
static int64_t wait_result = -11;
static unsigned wait_calls;
static unsigned wait_block;
static uint64_t poll_clock;
static unsigned poll_sleeps, poll_max_sleep, poll_ready_at;
static int poll_error, sleep_error;

int64_t lume_abi_host_call(unsigned number, uint64_t a, uint64_t b, uint64_t c)
{
    if (number == LUME_SYS_PROBE)
        return 0;
    if (number == LUME_SYS_CLOCK)
        return (int64_t)poll_clock;
    if (number == LUME_SYS_SLEEP)
    {
        assert(a > 0 && a <= 20 && !b && !c);
        ++poll_sleeps;
        if (a > poll_max_sleep)
            poll_max_sleep = (unsigned)a;
        poll_clock += a;
        return sleep_error;
    }
    if (number == LUME_SYS_FD)
    {
        assert(a == LUME_FD_POLL && c == sizeof(struct lume_fd_request));
        const struct lume_fd_request *request = (const void *)(uintptr_t)b;
        assert(!request->timeout_ms);
        return poll_error ? poll_error : poll_ready_at && poll_clock >= poll_ready_at;
    }
    if (number == LUME_SYS_CPU)
    {
        assert(!b && !c && a <= 2);
        return a == 2 ? 17 : a == 1 ? 3 : 4;
    }
    if (number == LUME_SYS_WAIT)
    {
        assert(!a && b == wait_block && !c);
        ++wait_calls;
        return wait_result;
    }
    assert(a == 3);
    if (number == LUME_SYS_STAT)
    {
        if (stat_error)
            return stat_error;
        *(struct lume_native_stat *)(uintptr_t)b = (struct lume_native_stat){1, size, kind, 0};
        return 0;
    }
    assert(number == LUME_SYS_SEEK);
    if (kind == LUME_NODE_CHAR)
        return -29;
    assert(c <= 2);
    position = (c == 0 ? 0 : c == 1 ? position : (int64_t)size) + (int64_t)b;
    return position;
}

int main(void)
{
    assert(lume_syscall(LUME_INFO, LUME_QUANTUM_MS, 0, 0) == 17);
    assert(lume_syscall(LUME_INFO, LUME_CORES, 0, 0) == 4);
    assert(lume_syscall(LUME_INFO, LUME_CORE, 0, 0) == 3);
    assert(lume_syscall(LUME_SEEK, 3, 0, LUME_SEEK_SET) == 0);
    assert(lume_syscall(LUME_SEEK, 3, 12, LUME_SEEK_SET) == 12);
    assert(lume_syscall(LUME_SEEK, 3, 1, LUME_SEEK_CUR) == -22 && position == 12);
    assert(lume_syscall(LUME_SEEK, 3, (uint32_t)-1, LUME_SEEK_END) == 11);
    assert(lume_syscall(LUME_SEEK, 3, (uint32_t)-13, LUME_SEEK_END) == -22 && position == 11);
    assert(lume_syscall(LUME_SEEK, 3, 0, 3) == -22);
    position = INT64_MAX;
    assert(lume_syscall(LUME_SEEK, 3, 1, LUME_SEEK_CUR) == -22 && position == INT64_MAX);
    size = INT64_MAX;
    assert(lume_syscall(LUME_SEEK, 3, 1, LUME_SEEK_END) == -22);
    size = UINT32_MAX;
    assert(lume_syscall(LUME_SEEK, 3, 0, LUME_SEEK_END) == -75);
    kind = LUME_NODE_CHAR;
    assert(lume_syscall(LUME_SEEK, 3, 0, LUME_SEEK_CUR) == -29);
    stat_error = -9;
    assert(lume_syscall(LUME_SEEK, 3, 0, LUME_SEEK_SET) == -9);
    assert(lume_syscall(LUME_WAITPID, UINT32_MAX, 0, 1) == 0 && wait_calls == 1);
    wait_result = (INT64_C(4) << 32) | 7;
    assert(lume_syscall(LUME_WAITPID, UINT32_MAX, 0, 1) == 4 && wait_calls == 2);
    wait_block = 1;
    assert(lume_syscall(LUME_WAITPID, UINT32_MAX, 0, 0) == 4 && wait_calls == 3);
#ifdef _WIN32
    struct lume_fd_request *idle = VirtualAlloc((void *)(uintptr_t)0x10000000, sizeof(*idle),
                                                MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    assert(idle && (uintptr_t)idle <= UINT32_MAX);
#else
    struct lume_fd_request *idle = mmap((void *)(uintptr_t)0x10000000, sizeof(*idle),
                                        PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(idle != MAP_FAILED && (uintptr_t)idle <= UINT32_MAX);
#endif
    *idle = (struct lume_fd_request){
        .version = LUME_FD_VERSION, .bytes = sizeof(*idle), .fd = -1, .timeout_ms = 63};
    assert(!lume_syscall(LUME_FD, LUME_FD_POLL, (uint32_t)(uintptr_t)idle, sizeof(*idle)));
    assert(poll_clock == 63 && poll_sleeps <= 10 && poll_max_sleep <= 20);
    poll_clock = poll_sleeps = poll_max_sleep = 0;
    idle->timeout_ms = 3;
    assert(!lume_syscall(LUME_FD, LUME_FD_POLL, (uint32_t)(uintptr_t)idle, sizeof(*idle)));
    assert(poll_clock == 3 && poll_max_sleep <= 3);
    poll_clock = poll_sleeps = 0;
    idle->timeout_ms = UINT32_MAX;
    poll_ready_at = 100;
    assert(lume_syscall(LUME_FD, LUME_FD_POLL, (uint32_t)(uintptr_t)idle, sizeof(*idle)) == 1);
    assert(poll_clock >= 100 && poll_clock < 120);
    poll_clock = poll_sleeps = poll_ready_at = 0;
    poll_error = -9;
    assert(lume_syscall(LUME_FD, LUME_FD_POLL, (uint32_t)(uintptr_t)idle, sizeof(*idle)) == -9);
    assert(!poll_sleeps);
    poll_error = 0;
    sleep_error = -4;
    assert(lume_syscall(LUME_FD, LUME_FD_POLL, (uint32_t)(uintptr_t)idle, sizeof(*idle)) == -4);
#ifdef _WIN32
    assert(VirtualFree(idle, 0, MEM_RELEASE));
#else
    assert(!munmap(idle, sizeof(*idle)));
#endif
    puts(
        "PASS_COMMON_ABI seek bounds, signed offsets, overflow, access, streams and bounded FD waits");
    return 0;
}
