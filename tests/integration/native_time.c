/* SPDX-License-Identifier: BSD-3-Clause */
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include "lume/abi.h"
#include "lume/realtime_abi.h"
#include "lume/syscall_native.h"

int main(void)
{
    struct timeval value = {.tv_sec = 123, .tv_usec = 456};
    assert(gettimeofday(&value, NULL) < 0 && errno == EAGAIN);
    assert(!value.tv_sec && !value.tv_usec);
    struct timespec mono;
    assert(!clock_gettime(CLOCK_MONOTONIC, &mono));
    struct lume_realtime_request request = {.version = LUME_REALTIME_VERSION,
                                            .bytes = sizeof(request),
                                            .seconds = UINT64_C(1793664000),
                                            .nanoseconds = 123456000};
    assert(!lume_native_call(LUME_SYS_REALTIME, LUME_REALTIME_SET, (uintptr_t)&request,
                             sizeof(request)));
    assert(!gettimeofday(&value, NULL));
    assert(value.tv_sec >= 1793664000 && value.tv_sec < 1793664060);
    assert(value.tv_usec >= 0 && value.tv_usec < 1000000);
    struct timespec real;
    assert(!clock_gettime(CLOCK_REALTIME, &real));
    assert(real.tv_sec >= value.tv_sec && real.tv_nsec >= 0 && real.tv_nsec < 1000000000);
    puts("TIME_UNSET_ZERO_AND_VALID_WALL_CLOCK_PASS");
    return 0;
}
