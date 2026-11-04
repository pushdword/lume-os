/* SPDX-License-Identifier: BSD-3-Clause */
#include <sys/time.h>
#include <time.h>

int lume_fltk_monotonic_time(struct timeval *value, void *zone)
{
    (void)zone;
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now))
        return -1;
    value->tv_sec = now.tv_sec;
    value->tv_usec = now.tv_nsec / 1000;
    return 0;
}
