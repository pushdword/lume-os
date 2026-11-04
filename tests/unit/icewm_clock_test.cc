// SPDX-License-Identifier: BSD-3-Clause
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <stdio.h>

static bool synchronized;
static time_t utc;

static int test_clock(clockid_t id, struct timespec *result)
{
    if (id == CLOCK_MONOTONIC)
    {
        *result = {1234, 567000000};
        return 0;
    }
    if (id == CLOCK_REALTIME && synchronized)
    {
        *result = {utc, 0};
        return 0;
    }
    return -1;
}

static int test_timeofday(struct timeval *, void *)
{
    return -1;
}

#define LUME_NATIVE_CLOCK 1
#define clock_gettime test_clock
#define gettimeofday test_timeofday
#include "../../vendor/icewm/src/ytime.cc"
#undef clock_gettime
#undef gettimeofday

int main()
{
    timeval before = monotime();
    assert(before.tv_sec == 1234 && before.tv_usec == 567000);
    assert(walltime().tv_sec == 0);
    synchronized = true;
    utc = 1793628000;
    assert(walltime().tv_sec == utc);
    utc += 3600;
    timeval after = monotime();
    assert(before.tv_sec == after.tv_sec && before.tv_usec == after.tv_usec);
    puts("ICEWM_CLOCK_PASS");
}
