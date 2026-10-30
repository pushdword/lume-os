#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

static unsigned monotonic_calls, realtime_calls;

static int monotonic_probe(int clock, struct timespec *value)
{
    if (clock != CLOCK_MONOTONIC)
        return -1;
    ++monotonic_calls;
    value->tv_sec = 123;
    value->tv_nsec = 456000000;
    return 0;
}

static int realtime_probe(struct timeval *value, void *zone)
{
    (void)value;
    (void)zone;
    ++realtime_calls;
    return -1;
}

#define clock_gettime monotonic_probe
#define gettimeofday realtime_probe
#include <X11/Xos.h>

int main(void)
{
    struct timeval value = {0};
    if (X_GETTIMEOFDAY(&value) || monotonic_calls != 1 || realtime_calls || value.tv_sec != 123 ||
        value.tv_usec != 456000)
        return 1;
    puts("X11_MONOTONIC_PASS");
    return 0;
}
