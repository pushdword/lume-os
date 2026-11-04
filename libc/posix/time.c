/* SPDX-License-Identifier: BSD-3-Clause */
#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#define _POSIX_MONOTONIC_CLOCK 200809L
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>
#if defined(__aarch64__) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
#include "lume/syscall_native.h"
#include "lume/abi.h"
#include "lume/realtime_abi.h"
#include "errno_map.h"
#else
#include "api.h"
#endif

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
                int64_t elapsed =
                    (int64_t)(now.tv_sec - start.tv_sec) * 1000000000 + now.tv_nsec - start.tv_nsec;
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

int clock_gettime(clockid_t id, struct timespec *t)
{
    if (!t)
    {
        errno = EFAULT;
        return -1;
    }
#if defined(__aarch64__) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
    if (id == CLOCK_REALTIME)
    {
        struct lume_realtime_request request = {.version = LUME_REALTIME_VERSION,
                                                .bytes = sizeof(request)};
        int64_t result = lume_native_call(LUME_SYS_REALTIME, LUME_REALTIME_GET, (uintptr_t)&request,
                                          sizeof(request));
        if (result < 0)
        {
            errno = lume_errno_from_syscall((unsigned)-result);
            return -1;
        }
        t->tv_sec = (time_t)request.seconds;
        if (t->tv_sec < 0 || (uint64_t)t->tv_sec != request.seconds)
        {
            errno = EOVERFLOW;
            return -1;
        }
        t->tv_nsec = request.nanoseconds;
        return 0;
    }
#endif
    if (id != CLOCK_MONOTONIC)
    {
        errno = ENOSYS;
        return -1;
    }
#if defined(__aarch64__) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
    uint64_t ms = (uint64_t)lume_native_call(LUME_SYS_CLOCK, 0, 0, 0);
#else
    uint32_t ms = (uint32_t)lume_syscall(LUME_INFO, LUME_UPTIME, 0, 0);
#endif
    t->tv_sec = ms / 1000;
    t->tv_nsec = (ms % 1000) * 1000000;
    return 0;
}

int clock_getres(clockid_t id, struct timespec *resolution)
{
    if (id != CLOCK_MONOTONIC
#if defined(__aarch64__) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
        && id != CLOCK_REALTIME
#endif
    )
    {
        errno = EINVAL;
        return -1;
    }
    if (resolution)
        *resolution = (struct timespec){.tv_sec = 0, .tv_nsec = 1000000};
    return 0;
}

int gettimeofday(struct timeval *t, void *zone)
{
    (void)zone;
    if (!t)
    {
        errno = EFAULT;
        return -1;
    }
    *t = (struct timeval){0};
    struct timespec now;
    if (clock_gettime(CLOCK_REALTIME, &now))
        return -1;
    t->tv_sec = now.tv_sec;
    t->tv_usec = now.tv_nsec / 1000;
    return 0;
}
