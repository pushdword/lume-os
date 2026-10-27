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
static unsigned poll_calls, clock_calls, sleep_calls;
static uint64_t poll_timeout, poll_buffer;
static unsigned poll_count;
static int poll_result;
static unsigned native_calls, io_calls, io_number;
static uint64_t io_fd, io_buffer, io_bytes;
static int64_t io_result;

int64_t lume_abi_host_call(unsigned number, uint64_t a, uint64_t b, uint64_t c)
{
    ++native_calls;
    if (number == LUME_SYS_READ || number == LUME_SYS_WRITE)
    {
        ++io_calls;
        io_number = number;
        io_fd = a;
        io_buffer = b;
        io_bytes = c;
        return io_result;
    }
    if (number == LUME_SYS_PROBE)
        return 0;
    if (number == LUME_SYS_CLOCK)
    {
        ++clock_calls;
        return 0;
    }
    if (number == LUME_SYS_SLEEP)
    {
        ++sleep_calls;
        return 0;
    }
    if (number == LUME_SYS_FD)
    {
        assert(a == LUME_FD_WAIT && c == sizeof(struct lume_poll_request));
        const struct lume_poll_request *request = (const void *)(uintptr_t)b;
        assert(request->version == LUME_POLL_VERSION && request->bytes == sizeof(*request));
        assert(!request->flags && !request->signal_epoch);
        ++poll_calls;
        poll_timeout = request->timeout_ms;
        poll_buffer = request->buffer;
        poll_count = request->count;
        return poll_result;
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

static void io_forwarding(void)
{
    static const struct
    {
        uint32_t number, fd, buffer, bytes;
        int32_t result;
    } cases[] = {{LUME_READ, 3, 0x10000000, 16, 16},
                 {LUME_READ, 3, 0x10000000, 16, 7},
                 {LUME_FDWRITE, 3, 0x10000000, 16, 16},
                 {LUME_FDWRITE, 3, 0x10000000, 16, 5},
                 {LUME_READ, 3, 0x10000000, 16, 0},
                 {LUME_READ, 3, UINT32_MAX, 0, 0},
                 {LUME_FDWRITE, 3, UINT32_MAX, 0, 0},
                 {LUME_READ, UINT32_MAX, 0x10000000, 16, -LUME_EBADF},
                 {LUME_FDWRITE, UINT32_MAX, 0x10000000, 16, -LUME_EBADF},
                 {LUME_READ, 3, UINT32_MAX, 16, -LUME_EFAULT},
                 {LUME_FDWRITE, 3, UINT32_MAX, 16, -LUME_EFAULT},
                 {LUME_READ, UINT32_MAX, UINT32_MAX, 16, -LUME_EFAULT},
                 {LUME_READ, 3, 0x10000000, 16, -LUME_EAGAIN},
                 {LUME_FDWRITE, 3, 0x10000000, 16, -LUME_EPIPE},
                 {LUME_READ, 3, 0x10000000, 16, -4},
                 {LUME_FDWRITE, 3, 0, LUME_FILE_IO_MAX + 1, -LUME_EINVAL}};

    /* Native I/O determines errors for invalid buffers and descriptors. */
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
    {
        native_calls = io_calls = 0;
        io_result = cases[i].result;
        assert(lume_syscall(cases[i].number, cases[i].fd, cases[i].buffer, cases[i].bytes) ==
               cases[i].result);
        assert(native_calls == 1 && io_calls == 1);
        assert(io_number == (cases[i].number == LUME_READ ? LUME_SYS_READ : LUME_SYS_WRITE));
        assert(io_fd == cases[i].fd && io_buffer == cases[i].buffer && io_bytes == cases[i].bytes);
    }
}

int main(void)
{
    io_forwarding();
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
    idle->buffer = 0x12345670;
    idle->count = 3;
    assert(!lume_syscall(LUME_FD, LUME_FD_POLL, (uint32_t)(uintptr_t)idle, sizeof(*idle)));
    assert(poll_calls == 1 && poll_timeout == 63 && poll_count == 3 && poll_buffer == idle->buffer);
    idle->timeout_ms = 0;
    poll_result = 2;
    assert(lume_syscall(LUME_FD, LUME_FD_POLL, (uint32_t)(uintptr_t)idle, sizeof(*idle)) == 2);
    assert(poll_calls == 2 && !poll_timeout);
    idle->timeout_ms = UINT32_MAX;
    poll_result = 1;
    assert(lume_syscall(LUME_FD, LUME_FD_POLL, (uint32_t)(uintptr_t)idle, sizeof(*idle)) == 1);
    assert(poll_calls == 3 && poll_timeout == UINT64_MAX);
    poll_result = -9;
    assert(lume_syscall(LUME_FD, LUME_FD_POLL, (uint32_t)(uintptr_t)idle, sizeof(*idle)) == -9);
    poll_result = -4;
    assert(lume_syscall(LUME_FD, LUME_FD_POLL, (uint32_t)(uintptr_t)idle, sizeof(*idle)) == -4);
    assert(poll_calls == 5 && !clock_calls && !sleep_calls);
    idle->timeout_ms = 60001;
    assert(lume_syscall(LUME_FD, LUME_FD_POLL, (uint32_t)(uintptr_t)idle, sizeof(*idle)) == -22);
    idle->timeout_ms = 0;
    idle->flags = 1;
    assert(lume_syscall(LUME_FD, LUME_FD_POLL, (uint32_t)(uintptr_t)idle, sizeof(*idle)) == -22);
    idle->flags = 0;
    idle->count = LUME_FD_LIMIT + 1;
    assert(lume_syscall(LUME_FD, LUME_FD_POLL, (uint32_t)(uintptr_t)idle, sizeof(*idle)) == -22);
    assert(poll_calls == 5);
#ifdef _WIN32
    assert(VirtualFree(idle, 0, MEM_RELEASE));
#else
    assert(!munmap(idle, sizeof(*idle)));
#endif
    puts(
        "PASS_COMMON_ABI direct I/O, seek bounds, signed offsets, overflow, access and event FD waits");
    return 0;
}
