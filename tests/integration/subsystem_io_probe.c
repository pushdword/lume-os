#include "lume/app_support.h"
#include "fd.h"
#include <stdbool.h>
#include <stdint.h>

#define WRITERS 3u
#define CHILDREN (WRITERS + 1u)
#define BLOCKS 32u
#define BLOCK_BYTES 4096u
#define PIPE_BYTES 257u
#define REPORT_READY 1u
volatile uint64_t subsystem_io_progress;
static int payload[3] = {-1, -1, -1};
#define REPORT_DONE 2u
#define GUARD_BEFORE UINT64_C(0x5142554642454749)
#define GUARD_AFTER UINT64_C(0x51425546454e4421)
#define HASH_INITIAL UINT64_C(14695981039346656037)

static const char directory[] = "/data/subsystem-io-test";
static const char *const paths[WRITERS] = {"/data/subsystem-io-test/child-0",
                                           "/data/subsystem-io-test/child-1",
                                           "/data/subsystem-io-test/child-2"};

static struct
{
    uint64_t before;
    unsigned char data[BLOCK_BYTES];
    uint64_t after;
} io_buffer = {.before = GUARD_BEFORE, .after = GUARD_AFTER};

struct report
{
    uint32_t kind, id, line, pid;
    uint64_t value, bytes, address;
};

_Static_assert(sizeof(struct report) <= LUME_PIPE_ATOMIC, "atomic child report");

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

static void failure(unsigned line, unsigned child, unsigned child_line)
{
    puts("SUBSYSTEM_IO_FAIL line=");
    number(line);
    puts(" child=");
    number(child);
    puts(" child_line=");
    number(child_line);
    puts("\n");
}

static int close_one(int *fd)
{
    int result = 0;
    if (*fd >= 0)
    {
        result = (int)close(*fd);
        *fd = -1;
    }
    return result;
}

static int make_pipe(int descriptors[2])
{
    struct lume_fd_request request = {.version = LUME_FD_VERSION, .bytes = sizeof(request)};
    int result = (int)call(LUME_SYS_FD, LUME_FD_PIPE, (uintptr_t)&request, sizeof(request));
    if (!result)
    {
        descriptors[0] = request.fd;
        descriptors[1] = request.other;
    }
    return result;
}

static int64_t poll_one(int fd, uint64_t timeout, uint16_t *events)
{
    struct lume_pollfd record = {.fd = fd, .events = LUME_POLLIN};
    struct lume_poll_request request = {.version = LUME_POLL_VERSION,
                                        .bytes = sizeof(request),
                                        .buffer = (uintptr_t)&record,
                                        .timeout_ms = timeout,
                                        .count = 1};
    int64_t result = call(LUME_SYS_FD, LUME_FD_WAIT, (uintptr_t)&request, sizeof(request));
    *events = record.revents;
    return result;
}

static bool receive_report(int fd, struct report *report)
{
    unsigned char *out = (void *)report;
    size_t remaining = sizeof(*report);
    while (remaining)
    {
        int64_t result = read(fd, out, remaining);
        if (result == -4)
            continue;
        if (result <= 0 || (uint64_t)result > remaining)
            return false;
        out += result;
        remaining -= (size_t)result;
    }
    return true;
}

static bool guards_valid(void)
{
    return io_buffer.before == GUARD_BEFORE && io_buffer.after == GUARD_AFTER;
}

static unsigned char pattern(unsigned id, unsigned block, unsigned offset)
{
    return (unsigned char)((offset * 17u) ^ (offset >> 5) ^ (block * 31u) ^ ((id + 1u) * 73u));
}

static uint64_t checksum(uint64_t hash, unsigned char byte)
{
    return (hash ^ byte) * UINT64_C(1099511628211);
}

static uint64_t expected_checksum(unsigned id)
{
    uint64_t hash = HASH_INITIAL;
    for (unsigned block = 0; block < BLOCKS; ++block)
        for (unsigned i = 0; i < BLOCK_BYTES; ++i)
            hash = checksum(hash, pattern(id, block, i));
    return hash;
}

static unsigned writer(unsigned id, struct report *report)
{
    int fd = -1;
    unsigned line = 0;
    uint64_t written_hash = HASH_INITIAL, read_hash = HASH_INITIAL;
#define CHECK_IO(condition)                                                                        \
    do                                                                                             \
    {                                                                                              \
        if (!(condition))                                                                          \
        {                                                                                          \
            line = __LINE__;                                                                       \
            goto cleanup;                                                                          \
        }                                                                                          \
    } while (0)
    fd = (int)open(paths[id], LUME_O_RDWR | LUME_O_CREAT | LUME_O_EXCL);
    CHECK_IO(fd >= 0);
    for (unsigned block = 0; block < BLOCKS; ++block)
    {
        for (unsigned i = 0; i < BLOCK_BYTES; ++i)
        {
            io_buffer.data[i] = pattern(id, block, i);
            written_hash = checksum(written_hash, io_buffer.data[i]);
        }
        CHECK_IO(write(fd, io_buffer.data, BLOCK_BYTES) == BLOCK_BYTES && guards_valid());
    }
    CHECK_IO(call(LUME_SYS_SEEK, (unsigned)fd, 0, 0) == 0);
    for (unsigned block = 0; block < BLOCKS; ++block)
    {
        for (unsigned i = 0; i < BLOCK_BYTES; ++i)
            io_buffer.data[i] = (unsigned char)~pattern(id, block, i);
        CHECK_IO(read(fd, io_buffer.data, BLOCK_BYTES) == BLOCK_BYTES && guards_valid());
        for (unsigned i = 0; i < BLOCK_BYTES; ++i)
        {
            CHECK_IO(io_buffer.data[i] == pattern(id, block, i));
            read_hash = checksum(read_hash, io_buffer.data[i]);
        }
    }
    CHECK_IO(read(fd, io_buffer.data, 1) == 0 && written_hash == read_hash);
    report->value = read_hash;
    report->bytes = (uint64_t)BLOCKS * BLOCK_BYTES;
cleanup:
    if (close_one(&fd) && !line)
        line = __LINE__;
    return line;
#undef CHECK_IO
}

static unsigned progress(struct report *report, uint64_t *previous)
{
    int64_t at = call(LUME_SYS_CLOCK, 0, 0, 0);
    if (at < 0 || (uint64_t)at < *previous || call(LUME_SYS_GETPID, 0, 0, 0) != report->pid ||
        call(LUME_SYS_SLEEP, 1, 0, 0))
        return __LINE__;
    *previous = (uint64_t)at;
    ++subsystem_io_progress;
    return 0;
}

static int64_t fd_control(unsigned operation, int descriptor)
{
    struct lume_fd_request request = {
        .version = LUME_FD_VERSION, .bytes = sizeof(request), .fd = descriptor};
    return call(LUME_SYS_FD, operation, (uintptr_t)&request, sizeof(request));
}

static int descriptor_count(void)
{
    struct lume_fd_info info = {0};
    struct lume_fd_request request = {
        .version = LUME_FD_VERSION, .bytes = sizeof(request), .buffer = (uint32_t)(uintptr_t)&info};
    if (call(LUME_SYS_FD, LUME_FD_INFO, (uintptr_t)&request, sizeof(request)) ||
        info.version != LUME_FD_VERSION || info.bytes != sizeof(info))
        return -1;
    return (int)info.open;
}

static int close_payload(void)
{
    int result = 0;
    for (unsigned i = 0; i < 3; ++i)
        result |= close_one(&payload[i]);
    return result;
}

static unsigned pipe_transfer(void)
{
    uint16_t events = UINT16_MAX;
    int64_t began = call(LUME_SYS_CLOCK, 0, 0, 0);
    if (began < 0 || poll_one(payload[0], 17, &events) || events ||
        call(LUME_SYS_CLOCK, 0, 0, 0) - began < 17)
        return __LINE__;
    puts("SUBSYSTEM_IO_PIPE_BEGIN\n");
    for (unsigned i = 0; i < PIPE_BYTES; ++i)
        io_buffer.data[i] = pattern(WRITERS, 7, i);
    if (write(payload[1], io_buffer.data, PIPE_BYTES) != PIPE_BYTES || !guards_valid())
        return __LINE__;
    struct lume_pollfd records[2] = {{payload[0], LUME_POLLIN, 0}, {payload[2], LUME_POLLIN, 0}};
    struct lume_poll_request request = {.version = LUME_POLL_VERSION,
                                        .bytes = sizeof(request),
                                        .buffer = (uintptr_t)records,
                                        .timeout_ms = 1000,
                                        .count = 2};
    if (call(LUME_SYS_FD, LUME_FD_WAIT, (uintptr_t)&request, sizeof(request)) != 2 ||
        records[0].revents != LUME_POLLIN || records[1].revents != LUME_POLLIN)
        return __LINE__;
    for (unsigned i = 0; i < PIPE_BYTES; ++i)
        io_buffer.data[i] = 0;
    unsigned done = 0;
    while (done < PIPE_BYTES)
    {
        unsigned count = PIPE_BYTES - done;
        if (count > 17)
            count = 17;
        int64_t result = read(payload[2], io_buffer.data + done, count);
        if (result <= 0 || (uint64_t)result > count || !guards_valid())
            return __LINE__;
        done += (unsigned)result;
    }
    for (unsigned i = 0; i < PIPE_BYTES; ++i)
        if (io_buffer.data[i] != pattern(WRITERS, 7, i))
            return __LINE__;
    if (poll_one(payload[0], 0, &events) || events)
        return __LINE__;
    puts("SUBSYSTEM_IO_PIPE_PASS bytes=257 aliases=2 timeout=17\n");
    int count = descriptor_count();
    if (count != 8 || fd_control(LUME_FD_GETFD, payload[0]) ||
        fd_control(LUME_FD_GETFD, payload[2]) != LUME_FD_CLOEXEC ||
        fd_control(LUME_FD_GETFL, payload[0]) || fd_control(LUME_FD_GETFL, payload[2]) ||
        fd_control(LUME_FD_GETMODE, payload[0]) != LUME_O_RDONLY ||
        fd_control(LUME_FD_GETMODE, payload[1]) != LUME_O_WRONLY || descriptor_count() != count)
        return __LINE__;
    puts("SUBSYSTEM_IO_METADATA_PASS descriptors=8 unchanged\n");
    return 0;
}

static unsigned observer(int stop, struct report *report)
{
    int64_t initial = call(LUME_SYS_CLOCK, 0, 0, 0);
    if (initial < 0)
        return __LINE__;
    uint64_t previous = (uint64_t)initial;
    for (;;)
    {
        unsigned line = progress(report, &previous);
        if (line)
            return line;
        uint16_t events = 0;
        int64_t ready = poll_one(0, 5, &events);
        if (ready < 0 || (events & (LUME_POLLERR | LUME_POLLHUP | LUME_POLLNVAL)))
            return __LINE__;
        if (!ready)
            continue;
        unsigned char byte = 0;
        if (!(events & LUME_POLLIN) || call(LUME_SYS_TERMINAL, 1, 0, 0) != 1 ||
            read(0, &byte, 1) != 1 || byte != 'Q')
            return __LINE__;
        break;
    }
    puts("SUBSYSTEM_IO_ECHO_Q\n");
    unsigned line = pipe_transfer();
    if (line)
        return line;
    puts("SUBSYSTEM_IO_LONG_BEGIN "
         "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
         "fedcba9876543210fedcba9876543210fedcba9876543210fedcba9876543210"
         " SUBSYSTEM_IO_LONG_END\n");
    for (;;)
    {
        line = progress(report, &previous);
        if (line)
            return line;
        int64_t ready = call(LUME_SYS_TERMINAL, 5, 5, 0);
        if (ready == -LUME_EAGAIN)
            continue;
        unsigned char byte = 0;
        if (ready || call(LUME_SYS_TERMINAL, 1, 0, 0) != 1 || read(0, &byte, 1) != 1 || byte != 3)
            return __LINE__;
        break;
    }
    puts("SUBSYSTEM_IO_CTRL_C_BYTE=3\n");
    puts("SUBSYSTEM_IO_INPUT_PASS order=Q,3 bytes=2\n");
    for (;;)
    {
        line = progress(report, &previous);
        if (line)
            return line;
        uint16_t events = 0;
        int64_t ready = poll_one(stop, 0, &events);
        if (ready < 0 || (events & (LUME_POLLERR | LUME_POLLNVAL)))
            return __LINE__;
        if (ready)
        {
            unsigned char byte = 0;
            if (!(events & LUME_POLLIN) || read(stop, &byte, 1) != 1 || byte != 'S' ||
                descriptor_count() != 8 || !guards_valid())
                return __LINE__;
            report->value = subsystem_io_progress;
            report->bytes = PIPE_BYTES;
            return 0;
        }
    }
}

static _Noreturn void run_child(unsigned id, int start[2], int reports[2], int stop[2])
{
    struct report report = {.kind = REPORT_READY,
                            .id = id,
                            .pid = (uint32_t)call(LUME_SYS_GETPID, 0, 0, 0),
                            .address = (uintptr_t)io_buffer.data};
    int close_result = close_one(&start[1]);
    close_result |= close_one(&reports[0]);
    close_result |= close_one(&stop[1]);
    if (id < WRITERS)
    {
        close_result |= close_one(&stop[0]);
        close_result |= close_payload();
    }
    else
        close_result |= close_one(&start[0]);
    if (close_result)
        report.line = __LINE__;
    if (write(reports[1], &report, sizeof(report)) != sizeof(report))
        report.line = __LINE__;
    unsigned char token = 0;
    if (id < WRITERS && !report.line && (read(start[0], &token, 1) != 1 || token != 'G'))
        report.line = __LINE__;
    if (id < WRITERS && close_one(&start[0]) && !report.line)
        report.line = __LINE__;
    if (!report.line)
        report.line = id < WRITERS ? writer(id, &report) : observer(stop[0], &report);
    if ((close_one(&stop[0]) | close_payload()) && !report.line)
        report.line = __LINE__;
    report.kind = REPORT_DONE;
    if (id == WRITERS && report.line)
        failure(report.line, id, report.line);
    if (write(reports[1], &report, sizeof(report)) != sizeof(report))
        report.line = __LINE__;
    if (close_one(&reports[1]) && !report.line)
        report.line = __LINE__;
    (void)call(LUME_SYS_EXIT, report.line ? 1 : 0, 0, 0);
    for (;;)
    {
    }
}

static bool child_ok(unsigned pid)
{
    int64_t result;
    do
    {
        result = call(LUME_SYS_WAIT, pid, 1, 0);
    } while (result == -4);
    return result >= 0 && (uint32_t)((uint64_t)result >> 32) == pid && !(uint32_t)result;
}

int main(void)
{
    int start[2] = {-1, -1}, reports[2] = {-1, -1}, stop[2] = {-1, -1};
    unsigned children[CHILDREN] = {0};
    struct report report = {0};
    uint64_t sums[WRITERS] = {0}, clock_progress = 0;
    bool created = false;
    unsigned failure_line = 0, ready = 0, completed = 0;
#define REQUIRE(condition)                                                                         \
    do                                                                                             \
    {                                                                                              \
        if (!(condition))                                                                          \
        {                                                                                          \
            failure_line = __LINE__;                                                               \
            goto cleanup;                                                                          \
        }                                                                                          \
    } while (0)
    for (unsigned i = 0; i < BLOCK_BYTES; ++i)
        io_buffer.data[i] = 0xa5;
    REQUIRE(!call(LUME_SYS_MKDIR, (uintptr_t)directory, 0, 0));
    created = true;
    REQUIRE(!make_pipe(start) && !make_pipe(reports) && !make_pipe(stop) && !make_pipe(payload));
    struct lume_fd_request duplicate = {.version = LUME_FD_VERSION,
                                        .bytes = sizeof(duplicate),
                                        .fd = payload[0],
                                        .flags = LUME_FD_CLOEXEC};
    payload[2] = (int)call(LUME_SYS_FD, LUME_FD_DUP, (uintptr_t)&duplicate, sizeof(duplicate));
    REQUIRE(payload[2] >= 0);
    puts("SUBSYSTEM_IO_BEGIN writers=3 bytes=393216 same-VA buffers\n");
    for (unsigned id = 0; id < CHILDREN; ++id)
    {
        int64_t child = call(LUME_SYS_FORK, 0, 0, 0);
        REQUIRE(child >= 0);
        if (!child)
            run_child(id, start, reports, stop);
        children[id] = (unsigned)child;
    }
    REQUIRE(!close_one(&start[0]) && !close_one(&reports[1]) && !close_one(&stop[0]) &&
            !close_payload());
    for (unsigned i = 0; i < CHILDREN; ++i)
    {
        REQUIRE(receive_report(reports[0], &report));
        REQUIRE(report.kind == REPORT_READY && report.id < CHILDREN && !report.line &&
                report.pid == children[report.id] && !(ready & (1u << report.id)) &&
                report.address == (uintptr_t)io_buffer.data);
        ready |= 1u << report.id;
    }
    REQUIRE(ready == (1u << CHILDREN) - 1);
    puts("SUBSYSTEM_IO_READY observer=");
    number(children[WRITERS]);
    puts(" writers=");
    for (unsigned i = 0; i < WRITERS; ++i)
    {
        if (i)
            puts(",");
        number(children[i]);
    }
    puts("\n");
    REQUIRE(!call(LUME_SYS_SLEEP, 500, 0, 0));
    REQUIRE(write(start[1], "GGG", WRITERS) == WRITERS && !close_one(&start[1]));
    for (unsigned i = 0; i < WRITERS; ++i)
    {
        REQUIRE(receive_report(reports[0], &report));
        REQUIRE(report.kind == REPORT_DONE && report.id < WRITERS && !report.line &&
                report.pid == children[report.id] && !(completed & (1u << report.id)) &&
                report.bytes == (uint64_t)BLOCKS * BLOCK_BYTES &&
                report.value == expected_checksum(report.id));
        completed |= 1u << report.id;
        sums[report.id] = report.value;
    }
    REQUIRE(completed == (1u << WRITERS) - 1);
    REQUIRE(write(stop[1], "S", 1) == 1 && !close_one(&stop[1]));
    REQUIRE(receive_report(reports[0], &report));
    REQUIRE(report.kind == REPORT_DONE && report.id == WRITERS && !report.line &&
            report.pid == children[WRITERS] && report.value && report.bytes == PIPE_BYTES);
    clock_progress = report.value;
    REQUIRE(guards_valid());
    for (unsigned i = 0; i < BLOCK_BYTES; ++i)
        REQUIRE(io_buffer.data[i] == 0xa5);
cleanup:
    {
        int close_result = close_payload();
        close_result |= close_one(&start[0]);
        close_result |= close_one(&start[1]);
        close_result |= close_one(&reports[0]);
        close_result |= close_one(&reports[1]);
        close_result |= close_one(&stop[0]);
        close_result |= close_one(&stop[1]);
        if (close_result && !failure_line)
            failure_line = __LINE__;
    }
    for (unsigned i = 0; i < CHILDREN; ++i)
        if (children[i] && !child_ok(children[i]) && !failure_line)
            failure_line = __LINE__;
    if (created)
    {
        for (unsigned i = 0; i < WRITERS; ++i)
        {
            int64_t result = call(LUME_SYS_UNLINK, (uintptr_t)paths[i], 0, 0);
            if (result && result != -LUME_ENOENT && !failure_line)
                failure_line = __LINE__;
        }
        if (call(LUME_SYS_RMDIR, (uintptr_t)directory, 0, 0) && !failure_line)
            failure_line = __LINE__;
    }
    if (failure_line)
    {
        failure(failure_line, report.id, report.line);
        return 1;
    }
    for (unsigned id = 0; id < WRITERS; ++id)
    {
        puts("SUBSYSTEM_IO_CHILD_PASS child=");
        number(id);
        puts(" checksum=");
        number(sums[id]);
        puts("\n");
    }
    puts("SUBSYSTEM_IO_PASS writers=3 bytes=393216 clock_progress=");
    number(clock_progress);
    puts(" cleanup=ok\n");
    return 0;
#undef REQUIRE
}
