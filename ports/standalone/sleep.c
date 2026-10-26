#include "native.h"
#include <stddef.h>
#include <limits.h>
#include <time.h>

int nanosleep(const struct timespec *delay, struct timespec *remaining)
{
    if (!delay || delay->tv_sec < 0 || delay->tv_nsec < 0 || delay->tv_nsec >= 1000000000)
    {
        errno = EINVAL;
        return -1;
    }
    if ((uint64_t)delay->tv_sec > (UINT64_MAX - 999u) / 1000u)
    {
        errno = EOVERFLOW;
        return -1;
    }
    struct timespec before, after;
    if (clock_gettime(CLOCK_MONOTONIC, &before))
        return -1;
    uint64_t milliseconds =
        (uint64_t)delay->tv_sec * 1000u + (unsigned long)(delay->tv_nsec + 999999) / 1000000u;
    while (milliseconds)
    {
        unsigned step = milliseconds > 60000 ? 60000 : (unsigned)milliseconds;
        if (native_result(LUME_SYS_SLEEP, step, 0, 0) < 0)
        {
            if (remaining && clock_gettime(CLOCK_MONOTONIC, &after) == 0)
            {
                time_t seconds = after.tv_sec - before.tv_sec;
                long nanoseconds = after.tv_nsec - before.tv_nsec;
                if (nanoseconds < 0)
                {
                    --seconds;
                    nanoseconds += 1000000000;
                }
                *remaining = *delay;
                remaining->tv_sec -= seconds;
                remaining->tv_nsec -= nanoseconds;
                if (remaining->tv_nsec < 0)
                {
                    --remaining->tv_sec;
                    remaining->tv_nsec += 1000000000;
                }
                if (remaining->tv_sec < 0)
                    *remaining = (struct timespec){0, 0};
            }
            return -1;
        }
        milliseconds -= step;
    }
    if (remaining)
        *remaining = (struct timespec){0, 0};
    return 0;
}
