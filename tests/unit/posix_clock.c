/* SPDX-License-Identifier: BSD-3-Clause */
#include <assert.h>
#include <stdint.h>
#undef _DEFAULT_SOURCE
#undef _POSIX_C_SOURCE
#define LUME_ARCH_AMD64 1
#define LUME_ARCH_SYSCALL_NATIVE_H
#define clock_gettime tested_clock_gettime
#define clock_getres tested_clock_getres
#define gettimeofday tested_gettimeofday
#define nanosleep tested_nanosleep
#define usleep tested_usleep
static int64_t lume_native_call(uint64_t, uint64_t, uint64_t, uint64_t);
#include "../../libc/posix/time.c"

static int64_t realtime_result = -LUME_EAGAIN;
static int64_t uptime_result = 123456;
static unsigned uptime_calls;

int lume_errno_from_syscall(unsigned value)
{
    return value;
}

static int64_t lume_native_call(uint64_t number, uint64_t a, uint64_t b, uint64_t c)
{
    if (number == LUME_SYS_REALTIME)
    {
        assert(a == LUME_REALTIME_GET);
        assert(c == sizeof(struct lume_realtime_request));
        struct lume_realtime_request *request = (void *)(uintptr_t)b;
        assert(request->version == LUME_REALTIME_VERSION);
        assert(request->bytes == sizeof(*request));
        request->seconds = 1700000000;
        request->nanoseconds = 987654321;
        return realtime_result;
    }
    assert(number == LUME_SYS_CLOCK);
    assert(!a && !b && !c);
    ++uptime_calls;
    return uptime_result;
}

int main(void)
{
    struct timespec now;
    struct timeval value;
    assert(tested_clock_gettime(CLOCK_REALTIME, &now) == 0);
    assert(now.tv_sec == 123 && now.tv_nsec == 456000000);
    uptime_result += 37;
    assert(tested_gettimeofday(&value, NULL) == 0);
    assert(value.tv_sec == 123 && value.tv_usec == 493000);
    realtime_result = 0;
    unsigned calls = uptime_calls;
    assert(tested_clock_gettime(CLOCK_REALTIME, &now) == 0);
    assert(now.tv_sec == 1700000000 && now.tv_nsec == 987654321);
    assert(uptime_calls == calls);
    realtime_result = -LUME_EIO;
    assert(tested_clock_gettime(CLOCK_REALTIME, &now) == -1 && errno == EIO);
    assert(uptime_calls == calls);
    realtime_result = -LUME_EAGAIN;
    uptime_result = -LUME_EIO;
    assert(tested_gettimeofday(&value, NULL) == -1 && errno == EIO);
    assert(tested_clock_gettime(CLOCK_MONOTONIC, &now) == -1 && errno == EIO);
    assert(tested_clock_gettime((clockid_t)-1, &now) == -1 && errno == ENOSYS);
    return 0;
}
