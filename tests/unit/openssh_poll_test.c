#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "../../include/lume/poll.h"

typedef uint64_t sigset_t;
#define SIG_SETMASK 0

static int64_t native_call(unsigned, uint64_t, uint64_t, uint64_t);
static int native_result(int64_t);
static int restore_mask(int, const sigset_t *, sigset_t *);
#define sigprocmask restore_mask
#include "../../ports/openssh/lume/poll_wait.h"
#undef sigprocmask

static unsigned mask_calls, wait_calls, clock_calls, sleep_calls, restore_calls;
static uint64_t expected_timeout, epoch, pending;
static sigset_t original_mask, temporary_mask, current_mask;
static int mask_error, wait_result, restore_error;
static unsigned install_mask;
static struct lume_poll_request observed;
static uint16_t events[LUME_FD_LIMIT];

static int native_result(int64_t result)
{
    if (result >= 0)
        return (int)result;
    switch (-result)
    {
    case 4:
        errno = EINTR;
        break;
    case LUME_EIO:
        errno = EIO;
        break;
    case LUME_EBADF:
        errno = EBADF;
        break;
    case LUME_EFAULT:
        errno = EFAULT;
        break;
    case LUME_EINVAL:
        errno = EINVAL;
        break;
    default:
        assert(0);
        errno = EIO;
        break;
    }
    return -1;
}

static int64_t native_call(unsigned number, uint64_t a, uint64_t b, uint64_t c)
{
    if (number == LUME_SYS_CLOCK)
        ++clock_calls;
    if (number == LUME_SYS_SLEEP)
        ++sleep_calls;
    assert(number == LUME_SYS_SIGNAL || number == LUME_SYS_FD);
    if (number == LUME_SYS_SIGNAL)
    {
        assert(a == LUME_SIGNAL_MASK && c == sizeof(struct lume_signal_request));
        struct lume_signal_request *r = (void *)(uintptr_t)b;
        assert(r->version == LUME_SIGNAL_VERSION && r->bytes == sizeof(*r));
        assert(!r->operation && r->flags == install_mask);
        assert(r->mask == (install_mask ? (uint32_t)temporary_mask : 0));
        ++mask_calls;
        if (mask_error)
            return mask_error;
        r->mask = (uint32_t)original_mask;
        if (install_mask)
            current_mask = temporary_mask;
        r->delay_ms = epoch;
        r->interval_ms = pending;
        return 0;
    }
    assert(a == LUME_FD_WAIT && c == sizeof(struct lume_poll_request));
    const struct lume_poll_request *r = (const void *)(uintptr_t)b;
    assert(r->version == LUME_POLL_VERSION && r->bytes == sizeof(*r));
    assert(r->flags == LUME_POLL_SIGNAL_EPOCH && r->signal_epoch == epoch);
    assert(r->timeout_ms == expected_timeout && r->count <= LUME_FD_LIMIT);
    assert(current_mask == (install_mask ? temporary_mask : original_mask));
    observed = *r;
    ++wait_calls;
    struct lume_pollfd *p = (void *)(uintptr_t)r->buffer;
    for (unsigned i = 0; i < r->count; ++i)
    {
        assert(p[i].fd == (i ? 99 : 3));
        assert(p[i].events == (i ? LUME_POLLOUT : LUME_POLLIN | LUME_POLLOUT));
        assert(!p[i].revents);
        p[i].revents = events[i];
    }
    return wait_result;
}

static int restore_mask(int operation, const sigset_t *mask, sigset_t *old)
{
    assert(operation == SIG_SETMASK && mask && *mask == original_mask && !old);
    ++restore_calls;
    if (restore_error)
    {
        errno = restore_error;
        return -1;
    }
    current_mask = *mask;
    errno = EIO;
    return 0;
}

static void reset(void)
{
    mask_calls = wait_calls = clock_calls = sleep_calls = restore_calls = 0;
    epoch = UINT64_C(0x100000005);
    pending = install_mask = 0;
    mask_error = wait_result = restore_error = 0;
    original_mask = current_mask = 0x41;
    temporary_mask = 0x108;
    memset(&observed, 0, sizeof(observed));
    memset(events, 0, sizeof(events));
    errno = ERANGE;
}

static void ready_and_timeout(void)
{
    reset();
    struct pollfd fds[] = {{3, POLLIN | POLLOUT | POLLPRI, 123}, {99, POLLOUT, 123}};
    struct timespec delay = {123, 456000001};
    expected_timeout = 123457;
    install_mask = 1;
    wait_result = 2;
    events[0] = LUME_POLLIN | LUME_POLLOUT | LUME_POLLERR | LUME_POLLHUP;
    events[1] = LUME_POLLNVAL;
    assert(lume_ssh_ppoll(fds, 2, &delay, &temporary_mask) == 2);
    assert(mask_calls == 1 && wait_calls == 1 && restore_calls == 1);
    assert(!clock_calls && !sleep_calls && current_mask == original_mask && errno == ERANGE);
    assert(fds[0].revents == (POLLIN | POLLOUT | POLLERR | POLLHUP));
    assert(fds[1].revents == POLLNVAL);
    assert(observed.buffer && observed.count == 2);

    static const struct
    {
        time_t seconds;
        long nanoseconds;
        uint64_t milliseconds;
    } times[] = {{0, 0, 0},
                 {0, 1, 1},
                 {0, 999999, 1},
                 {1, 999999999, 2000},
                 {120, 0, 120000},
                 {(time_t)UINT64_C(0x100000000), 0, UINT64_C(0x100000000) * 1000},
                 {(time_t)(UINT64_MAX / 1000), 614000000, UINT64_MAX - 1}};

    for (unsigned i = 0; i < sizeof(times) / sizeof(times[0]); ++i)
    {
        reset();
        delay = (struct timespec){times[i].seconds, times[i].nanoseconds};
        expected_timeout = times[i].milliseconds;
        fds[0] = (struct pollfd){3, POLLIN | POLLOUT, 123};
        assert(!lume_ssh_ppoll(fds, 1, &delay, NULL));
        assert(mask_calls == 1 && wait_calls == 1 && !restore_calls && !fds[0].revents);
        assert(!clock_calls && !sleep_calls && errno == ERANGE);
    }
    reset();
    expected_timeout = UINT64_MAX;
    assert(!lume_ssh_ppoll(NULL, 0, NULL, NULL));
    assert(wait_calls == 1 && !observed.count && !restore_calls);
    reset();
    expected_timeout = 120001;
    assert(!lume_ssh_poll(NULL, 0, 120001) && wait_calls == 1);
    reset();
    expected_timeout = UINT64_MAX;
    assert(!lume_ssh_poll(NULL, 0, -1) && wait_calls == 1);
    reset();
    expected_timeout = 0;
    assert(!lume_ssh_poll(NULL, 0, 0) && wait_calls == 1);
}

static void interruption_and_errors(void)
{
    reset();
    struct pollfd fds = {3, POLLIN | POLLOUT, 123};
    struct timespec delay = {1, 0};
    expected_timeout = 1000;
    install_mask = 1;
    pending = 1;
    assert(lume_ssh_ppoll(&fds, 1, &delay, &temporary_mask) == -1 && errno == EINTR);
    assert(mask_calls == 1 && !wait_calls && restore_calls == 1 && !fds.revents);
    assert(current_mask == original_mask);

    reset();
    install_mask = 1;
    expected_timeout = 1000;
    wait_result = -4;
    events[0] = LUME_POLLIN;
    assert(lume_ssh_ppoll(&fds, 1, &delay, &temporary_mask) == -1 && errno == EINTR);
    assert(wait_calls == 1 && restore_calls == 1 && current_mask == original_mask);
    assert(!fds.revents && !clock_calls && !sleep_calls);

    reset();
    install_mask = 1;
    expected_timeout = 1000;
    wait_result = -LUME_EIO;
    events[0] = LUME_POLLIN;
    assert(lume_ssh_ppoll(&fds, 1, &delay, &temporary_mask) == -1 && errno == EIO);
    assert(wait_calls == 1 && restore_calls == 1 && !fds.revents);

    reset();
    install_mask = 1;
    expected_timeout = 1000;
    wait_result = 1;
    events[0] = LUME_POLLIN;
    restore_error = EACCES;
    assert(lume_ssh_ppoll(&fds, 1, &delay, &temporary_mask) == -1 && errno == EACCES);
    assert(wait_calls == 1 && restore_calls == 1 && !fds.revents);

    reset();
    mask_error = -LUME_EFAULT;
    fds.revents = 123;
    assert(lume_ssh_ppoll(&fds, 1, &delay, NULL) == -1 && errno == EFAULT);
    assert(mask_calls == 1 && !wait_calls && !restore_calls && fds.revents == 123);
}

static void invalid_arguments(void)
{
    static const struct
    {
        time_t seconds;
        long nanoseconds;
        int error;
    } times[] = {{-1, 0, EINVAL},
                 {0, -1, EINVAL},
                 {0, 1000000000, EINVAL},
                 {(time_t)(UINT64_MAX / 1000 + 1), 0, EOVERFLOW},
                 {(time_t)(UINT64_MAX / 1000), 615000000, EOVERFLOW},
                 {(time_t)INT64_MAX, 0, EOVERFLOW}};

    for (unsigned i = 0; i < sizeof(times) / sizeof(times[0]); ++i)
    {
        reset();
        struct timespec delay = {times[i].seconds, times[i].nanoseconds};
        assert(lume_ssh_ppoll(NULL, 0, &delay, &temporary_mask) == -1 && errno == times[i].error);
        assert(!mask_calls && !wait_calls && !restore_calls);
    }
    reset();
    assert(lume_ssh_ppoll(NULL, 1, NULL, NULL) == -1 && errno == EINVAL);
    assert(lume_ssh_ppoll(NULL, LUME_FD_LIMIT + 1, NULL, NULL) == -1 && errno == EINVAL);
    assert(lume_ssh_poll(NULL, 0, -2) == -1 && errno == EINVAL);
    assert(!mask_calls && !wait_calls && !restore_calls && !clock_calls && !sleep_calls);
}

int main(void)
{
    ready_and_timeout();
    interruption_and_errors();
    invalid_arguments();
    puts("PASS_OPENSSH_POLL native wait, signal epoch, mask restoration and timeout bounds");
    return 0;
}
