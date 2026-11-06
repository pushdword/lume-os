#ifndef LUME_ABSOLUTE_TIME_H
#define LUME_ABSOLUTE_TIME_H
#include <errno.h>
#include <stdint.h>
#include <time.h>

static int timespec_deadline_ms(const struct timespec *time, uint64_t *milliseconds)
{
    if (!time || time->tv_nsec < 0 || time->tv_nsec >= 1000000000L)
        return EINVAL;
    if (time->tv_sec < 0)
    {
        *milliseconds = 0;
        return 0;
    }
    uint64_t seconds = (uint64_t)time->tv_sec;
    uint64_t fraction = ((uint64_t)time->tv_nsec + 999999) / 1000000;
    if (seconds > UINT64_MAX / 1000 || fraction > UINT64_MAX - seconds * 1000)
        return EOVERFLOW;
    *milliseconds = seconds * 1000 + fraction;
    return 0;
}
#endif
