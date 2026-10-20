/* SPDX-License-Identifier: BSD-3-Clause */
#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#define _POSIX_MONOTONIC_CLOCK 200809L
#include "api.h"
#include <sys/time.h>
#include <time.h>
#include <errno.h>
/* There is no wall-clock service yet. Monotonic requests use the kernel
 * hardware counter. Realtime is reported unavailable rather than fabricated. */
int clock_gettime(clockid_t id,struct timespec *t){
 if(id!=CLOCK_MONOTONIC){errno=ENOSYS;return -1;}
 uint32_t ms=(uint32_t)lume_syscall(LUME_INFO,LUME_UPTIME,0,0);
 t->tv_sec=ms/1000;t->tv_nsec=(ms%1000)*1000000;return 0;
}
int gettimeofday(struct timeval*t,void*zone){(void)t;(void)zone;errno=ENOSYS;return -1;}
