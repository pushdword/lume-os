#include "lume/app_support.h"
#include "fd.h"
#include "lume/socket.h"
#include "lume/signal.h"
#include "lume/process.h"
#include <stdbool.h>
#include <stdint.h>

#define TEST_SIGNAL 30u
#define ALARM_SIGNAL 14u
#define WAIT_MS 2000u

static volatile unsigned delivered, alarms, unexpected;

static struct
{
    uint64_t before;
    struct lume_pollfd records[2];
    uint64_t after;
} private_output;

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

static int failure(const char *name, unsigned line)
{
    puts("COMMON_EVENTS_FAIL ");
    puts(name);
    puts(" line=");
    number(line);
    puts("\n");
    return 1;
}

#define REQUIRE(condition, name)                                                                   \
    do                                                                                             \
    {                                                                                              \
        if (!(condition))                                                                          \
            return failure(name, __LINE__);                                                        \
    } while (0)

static _Noreturn void child_exit(unsigned status)
{
    (void)call(LUME_SYS_EXIT, status, 0, 0);
    for (;;)
    {
    }
}

#define CHILD_REQUIRE(condition, name)                                                             \
    do                                                                                             \
    {                                                                                              \
        if (!(condition))                                                                          \
            child_exit((unsigned)failure(name, __LINE__));                                         \
    } while (0)

static uint64_t clock_ms(void)
{
    return (uint64_t)call(LUME_SYS_CLOCK, 0, 0, 0);
}

static int64_t fd_wait(struct lume_pollfd *records, unsigned count, uint64_t timeout,
                       unsigned flags, uint64_t epoch)
{
    struct lume_poll_request request = {.version = LUME_POLL_VERSION,
                                        .bytes = sizeof(request),
                                        .buffer = (uintptr_t)records,
                                        .timeout_ms = timeout,
                                        .signal_epoch = epoch,
                                        .count = count,
                                        .flags = flags};
    return call(LUME_SYS_FD, LUME_FD_WAIT, (uintptr_t)&request, sizeof(request));
}

static int64_t old_poll(struct lume_pollfd *records, unsigned count, unsigned timeout)
{
    if ((uintptr_t)records > UINT32_MAX)
        return -LUME_EFAULT;
    struct lume_fd_request request = {.version = LUME_FD_VERSION,
                                      .bytes = sizeof(request),
                                      .buffer = (uint32_t)(uintptr_t)records,
                                      .count = count,
                                      .timeout_ms = timeout};
    return call(LUME_SYS_FD, LUME_FD_POLL, (uintptr_t)&request, sizeof(request));
}

static int make_pair(int descriptors[2], bool socket)
{
    if (socket)
    {
        struct lume_socket_request request = {
            .version = LUME_SOCKET_VERSION, .bytes = sizeof(request), .family = 1, .type = 1};
        int result =
            (int)call(LUME_SYS_SOCKET, LUME_SOCKET_PAIR, (uintptr_t)&request, sizeof(request));
        if (!result)
        {
            descriptors[0] = request.fd;
            descriptors[1] = request.other;
        }
        return result;
    }
    struct lume_fd_request request = {.version = LUME_FD_VERSION, .bytes = sizeof(request)};
    int result = (int)call(LUME_SYS_FD, LUME_FD_PIPE, (uintptr_t)&request, sizeof(request));
    if (!result)
    {
        descriptors[0] = request.fd;
        descriptors[1] = request.other;
    }
    return result;
}

static bool child_ok(unsigned pid)
{
    int64_t result;
    do
    {
        result = call(LUME_SYS_WAIT, pid, 1, 0);
    } while (result == -4);
    return result >= 0 && (unsigned)((uint64_t)result >> 32) == pid && !(uint32_t)result;
}

static bool ready_byte(int fd, unsigned char *byte)
{
    struct lume_pollfd record = {.fd = fd, .events = LUME_POLLIN};
    return fd_wait(&record, 1, WAIT_MS, 0, 0) == 1 && (record.revents & LUME_POLLIN) &&
           read(fd, byte, 1) == 1;
}

static int timeouts(void)
{
    struct lume_pollfd ignored = {.fd = -1, .events = LUME_POLLIN, .revents = UINT16_MAX};
    REQUIRE(fd_wait(&ignored, 1, 0, 0, 0) == 0 && !ignored.revents, "timeout-zero");
    uint64_t start = clock_ms();
    REQUIRE(fd_wait(NULL, 0, 63, 0, 0) == 0, "timeout-empty");
    REQUIRE(clock_ms() - start >= 63, "timeout-duration");
    REQUIRE(fd_wait(NULL, LUME_FD_LIMIT + 1, 0, 0, 0) == -LUME_EINVAL, "count-limit");
    REQUIRE(fd_wait((void *)(uintptr_t)UINT64_MAX, 1, 0, 0, 0) == -LUME_EFAULT, "output-range");
    return 0;
}

static int delayed_pair(bool socket, bool legacy)
{
    int descriptors[2];
    REQUIRE(!make_pair(descriptors, socket), "pair-create");
    int64_t child = call(LUME_SYS_FORK, 0, 0, 0);
    REQUIRE(child > -1, "pair-fork");
    if (!child)
    {
        CHILD_REQUIRE(!close(descriptors[0]), "writer-close-read");
        CHILD_REQUIRE(call(LUME_SYS_SLEEP, 35, 0, 0) == 0, "writer-delay");
        unsigned char byte = socket ? 'S' : legacy ? 'L' : 'P';
        CHILD_REQUIRE(write(descriptors[1], &byte, 1) == 1, "writer-byte");
        CHILD_REQUIRE(!close(descriptors[1]), "writer-close-write");
        child_exit(0);
    }
    REQUIRE(!close(descriptors[1]), "reader-close-write");
    struct lume_pollfd record = {.fd = descriptors[0], .events = LUME_POLLIN};
    int64_t result = legacy ? old_poll(&record, 1, WAIT_MS) : fd_wait(&record, 1, WAIT_MS, 0, 0);
    REQUIRE(result == 1 && (record.revents & LUME_POLLIN), "delayed-readable");
    unsigned char byte = 0;
    REQUIRE(read(descriptors[0], &byte, 1) == 1 && byte == (socket   ? 'S'
                                                            : legacy ? 'L'
                                                                     : 'P'),
            "delayed-payload");
    record.revents = 0;
    result = legacy ? old_poll(&record, 1, WAIT_MS) : fd_wait(&record, 1, WAIT_MS, 0, 0);
    REQUIRE(result == 1 && (record.revents & LUME_POLLHUP), "delayed-hangup");
    REQUIRE(read(descriptors[0], &byte, 1) == 0, "delayed-eof");
    REQUIRE(!close(descriptors[0]) && child_ok((unsigned)child), "delayed-reap");
    return 0;
}

static int pipe_events(void)
{
    return delayed_pair(false, false);
}

static int legacy_events(void)
{
    return delayed_pair(false, true);
}

static int unix_events(void)
{
    return delayed_pair(true, false);
}

static bool sleeping(unsigned pid)
{
    struct lume_task_info info = {0};
    return call(LUME_SYS_PROCESS, pid - 1, (uintptr_t)&info, sizeof(info)) == 1 &&
           info.pid == pid && info.state == 5;
}

static bool private_correct(int fd, unsigned slot)
{
    unsigned other = slot ^ 1;
    return private_output.before == UINT64_C(0x3156415052495641) &&
           private_output.after == UINT64_C(0x3256415052495641) &&
           private_output.records[slot].fd == fd &&
           private_output.records[slot].events == LUME_POLLIN &&
           (private_output.records[slot].revents & LUME_POLLIN) &&
           !(private_output.records[slot].revents & (LUME_POLLERR | LUME_POLLNVAL)) &&
           private_output.records[other].fd == -1 &&
           private_output.records[other].events == LUME_POLLIN &&
           !private_output.records[other].revents;
}

static int private_outputs(void)
{
    int parent_pipe[2], child_pipe[2], reports[2];
    REQUIRE(!make_pair(parent_pipe, false) && !make_pair(child_pipe, false) &&
                !make_pair(reports, false),
            "private-pipes");
    unsigned parent = (unsigned)call(LUME_SYS_GETPID, 0, 0, 0);
    private_output.before = UINT64_C(0x3156415052495641);
    private_output.after = UINT64_C(0x3256415052495641);
    int64_t child = call(LUME_SYS_FORK, 0, 0, 0);
    REQUIRE(child >= 0, "private-reader-fork");
    if (!child)
    {
        CHILD_REQUIRE(!close(parent_pipe[0]) && !close(parent_pipe[1]) && !close(child_pipe[1]) &&
                          !close(reports[0]),
                      "private-reader-close");
        private_output.records[0] = (struct lume_pollfd){child_pipe[0], LUME_POLLIN, UINT16_MAX};
        private_output.records[1] = (struct lume_pollfd){-1, LUME_POLLIN, UINT16_MAX};
        CHILD_REQUIRE(write(reports[1], "C", 1) == 1, "private-reader-barrier");
        CHILD_REQUIRE(fd_wait(private_output.records, 2, WAIT_MS, 0, 0) == 1 &&
                          private_correct(child_pipe[0], 0),
                      "private-reader-output");
        unsigned char byte = 0;
        CHILD_REQUIRE(read(child_pipe[0], &byte, 1) == 1 && byte == 'C', "private-reader-byte");
        CHILD_REQUIRE(!close(child_pipe[0]), "private-reader-close-input");
        CHILD_REQUIRE(write(reports[1], "O", 1) == 1 && !close(reports[1]),
                      "private-reader-report");
        child_exit(0);
    }
    int64_t writer = call(LUME_SYS_FORK, 0, 0, 0);
    REQUIRE(writer >= 0, "private-writer-fork");
    if (!writer)
    {
        CHILD_REQUIRE(!close(parent_pipe[0]) && !close(child_pipe[0]) && !close(reports[1]),
                      "private-writer-close");
        unsigned char first = 0, second = 0;
        CHILD_REQUIRE(ready_byte(reports[0], &first) && ready_byte(reports[0], &second) &&
                          ((first == 'P' && second == 'C') || (first == 'C' && second == 'P')),
                      "private-writer-barrier");
        CHILD_REQUIRE(!close(reports[0]), "private-writer-close-report");
        uint64_t start = clock_ms();
        while (!(sleeping(parent) && sleeping((unsigned)child)) && clock_ms() - start < WAIT_MS)
            CHILD_REQUIRE(!call(LUME_SYS_SLEEP, 1, 0, 0), "private-writer-state-delay");
        CHILD_REQUIRE(sleeping(parent) && sleeping((unsigned)child), "private-waiting-states");
        CHILD_REQUIRE(!call(LUME_SYS_SLEEP, 20, 0, 0), "private-writer-parent-delay");
        CHILD_REQUIRE(write(parent_pipe[1], "P", 1) == 1 && !close(parent_pipe[1]),
                      "private-writer-parent");
        CHILD_REQUIRE(!call(LUME_SYS_SLEEP, 20, 0, 0), "private-writer-child-delay");
        CHILD_REQUIRE(write(child_pipe[1], "C", 1) == 1 && !close(child_pipe[1]),
                      "private-writer-child");
        child_exit(0);
    }
    REQUIRE(!close(parent_pipe[1]) && !close(child_pipe[0]) && !close(child_pipe[1]),
            "private-parent-close");
    private_output.records[0] = (struct lume_pollfd){-1, LUME_POLLIN, UINT16_MAX};
    private_output.records[1] = (struct lume_pollfd){parent_pipe[0], LUME_POLLIN, UINT16_MAX};
    REQUIRE(write(reports[1], "P", 1) == 1 && !close(reports[1]), "private-parent-barrier");
    REQUIRE(fd_wait(private_output.records, 2, WAIT_MS, 0, 0) == 1 &&
                private_correct(parent_pipe[0], 1),
            "private-parent-output");
    unsigned char byte = 0;
    REQUIRE(read(parent_pipe[0], &byte, 1) == 1 && byte == 'P', "private-parent-byte");
    REQUIRE(ready_byte(reports[0], &byte) && byte == 'O', "private-parent-report");
    REQUIRE(child_ok((unsigned)child) && child_ok((unsigned)writer), "private-parent-reap");
    REQUIRE(private_correct(parent_pipe[0], 1), "private-parent-retained-output");
    REQUIRE(!close(parent_pipe[0]) && !close(reports[0]), "private-parent-release");
    return 0;
}

__attribute__((aligned(16))) static void signal_handler(unsigned number)
{
    if (number == TEST_SIGNAL)
        ++delivered;
    else if (number == ALARM_SIGNAL)
        ++alarms;
    else
        ++unexpected;
}

__attribute__((aligned(16))) static _Noreturn void restore_signal(void)
{
    (void)call(LUME_SYS_SIGNAL, LUME_SIGNAL_RETURN, 0, 0);
    child_exit(250);
}

static int signal_call(unsigned operation, struct lume_signal_request *request)
{
    request->version = LUME_SIGNAL_VERSION;
    request->bytes = sizeof(*request);
    return (int)call(LUME_SYS_SIGNAL, operation, (uintptr_t)request, sizeof(*request));
}

static int install_handler(unsigned number, struct lume_signal_request *old)
{
    *old = (struct lume_signal_request){.number = number,
                                        .operation = 1,
                                        .handler = (uintptr_t)signal_handler,
                                        .restorer = (uintptr_t)restore_signal};
    return signal_call(LUME_SIGNAL_ACTION, old);
}

static int signal_events(void)
{
    struct lume_signal_request old_user, old_alarm, mask = {0};
    REQUIRE(!signal_call(LUME_SIGNAL_MASK, &mask), "signal-mask-query");
    uint32_t original = mask.mask;
    mask = (struct lume_signal_request){
        .flags = 1, .mask = original & ~((1u << TEST_SIGNAL) | (1u << ALARM_SIGNAL))};
    REQUIRE(!signal_call(LUME_SIGNAL_MASK, &mask), "signal-active-mask");
    REQUIRE(!install_handler(TEST_SIGNAL, &old_user) && !install_handler(ALARM_SIGNAL, &old_alarm),
            "signal-actions");
    mask = (struct lume_signal_request){0};
    REQUIRE(!signal_call(LUME_SIGNAL_MASK, &mask), "signal-epoch-query");
    uint64_t epoch = mask.delay_ms;
    struct lume_signal_request send = {.number = TEST_SIGNAL,
                                       .pid = (int32_t)call(LUME_SYS_GETPID, 0, 0, 0)};
    REQUIRE(!signal_call(LUME_SIGNAL_SEND, &send) && delivered == 1 && !unexpected,
            "signal-handler-return");
    REQUIRE(fd_wait(NULL, 0, 0, LUME_POLL_SIGNAL_EPOCH, epoch) == -4, "signal-stale-epoch");

    mask = (struct lume_signal_request){.flags = 1, .mask = original | (1u << TEST_SIGNAL)};
    REQUIRE(!signal_call(LUME_SIGNAL_MASK, &mask), "signal-block");
    uint32_t active = mask.mask;
    epoch = mask.delay_ms;
    send = (struct lume_signal_request){.number = TEST_SIGNAL,
                                        .pid = (int32_t)call(LUME_SYS_GETPID, 0, 0, 0)};
    REQUIRE(!signal_call(LUME_SIGNAL_SEND, &send) && delivered == 1, "signal-masked-pending");
    uint64_t start = clock_ms();
    REQUIRE(fd_wait(NULL, 0, 63, LUME_POLL_SIGNAL_EPOCH, epoch) == 0 && delivered == 1 &&
                clock_ms() - start >= 63,
            "signal-masked-timeout");
    mask = (struct lume_signal_request){.flags = 1, .mask = active};
    REQUIRE(!signal_call(LUME_SIGNAL_MASK, &mask) && delivered == 2, "signal-unblock");

    mask = (struct lume_signal_request){0};
    REQUIRE(!signal_call(LUME_SIGNAL_MASK, &mask), "alarm-epoch-query");
    struct lume_signal_request timer = {.delay_ms = 100};
    REQUIRE(!signal_call(LUME_SIGNAL_TIMER, &timer), "alarm-start");
    REQUIRE(fd_wait(NULL, 0, WAIT_MS, LUME_POLL_SIGNAL_EPOCH, mask.delay_ms) == -4 && alarms == 1 &&
                !unexpected,
            "alarm-wait-interrupt");
    timer = (struct lume_signal_request){0};
    REQUIRE(!signal_call(LUME_SIGNAL_TIMER, &timer), "alarm-stop");
    REQUIRE(!signal_call(LUME_SIGNAL_ACTION, &old_user) &&
                !signal_call(LUME_SIGNAL_ACTION, &old_alarm),
            "signal-restore-actions");
    mask = (struct lume_signal_request){.flags = 1, .mask = original};
    REQUIRE(!signal_call(LUME_SIGNAL_MASK, &mask), "signal-restore-mask");
    return 0;
}

int main(int argc, char **argv)
{
    static const struct
    {
        const char *name;
        int (*run)(void);
    } tests[] = {{"timeouts", timeouts}, {"pipe", pipe_events},        {"legacy", legacy_events},
                 {"unix", unix_events},  {"private", private_outputs}, {"signals", signal_events}};

    bool selected = false;
    if (argc > 2)
        return failure("arguments", __LINE__);
    for (unsigned i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i)
        if (argc == 1 || eq(argv[1], "all") || eq(argv[1], tests[i].name))
        {
            selected = true;
            puts("COMMON_EVENTS_BEGIN ");
            puts(tests[i].name);
            puts("\n");
            if (tests[i].run())
                return 1;
            puts("COMMON_EVENTS_PASS ");
            puts(tests[i].name);
            puts("\n");
        }
    if (!selected)
        return failure("mode", __LINE__);
    puts("COMMON_EVENTS_ALL_PASS\n");
    return 0;
}
