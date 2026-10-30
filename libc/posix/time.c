/* SPDX-License-Identifier: BSD-3-Clause */
#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#define _POSIX_MONOTONIC_CLOCK 200809L
#include "api.h"
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>

int nanosleep(const struct timespec *delay, struct timespec *remaining)
{
    if (!delay || delay->tv_sec < 0 || delay->tv_nsec < 0 || delay->tv_nsec >= 1000000000L)
    {
        errno = EINVAL;
        return -1;
    }
    struct timespec start, now;
    if (clock_gettime(CLOCK_MONOTONIC, &start))
        return -1;
    int64_t milliseconds = (int64_t)delay->tv_sec * 1000 + (delay->tv_nsec + 999999) / 1000000;
    while (milliseconds)
    {
        int step = milliseconds > 60000 ? 60000 : (int)milliseconds;
        if (poll(NULL, 0, step) < 0)
        {
            if (remaining && !clock_gettime(CLOCK_MONOTONIC, &now))
            {
                int64_t elapsed = (int64_t)(now.tv_sec - start.tv_sec) * 1000000000 +
                    now.tv_nsec - start.tv_nsec;
                int64_t left = (int64_t)delay->tv_sec * 1000000000 + delay->tv_nsec - elapsed;
                if (left < 0)
                    left = 0;
                *remaining = (struct timespec){left / 1000000000, left % 1000000000};
            }
            return -1;
        }
        milliseconds -= step;
    }
    if (remaining)
        *remaining = (struct timespec){0};
    return 0;
}

int usleep(useconds_t usec)
{
    struct timespec delay = {usec / 1000000, (usec % 1000000) * 1000};
    return nanosleep(&delay, NULL);
}

/* There is no wall-clock service yet. Monotonic requests use the kernel
 * hardware counter. Realtime is reported unavailable rather than fabricated. */
int clock_gettime(clockid_t id, struct timespec *t)
{
    if (id != CLOCK_MONOTONIC)
    {
        errno = ENOSYS;
        return -1;
    }
    uint32_t ms = (uint32_t)lume_syscall(LUME_INFO, LUME_UPTIME, 0, 0);
    t->tv_sec = ms / 1000;
    t->tv_nsec = (ms % 1000) * 1000000;
    return 0;
}

int gettimeofday(struct timeval *t, void *zone)
{
    (void)t;
    (void)zone;
    errno = ENOSYS;
    return -1;
}
