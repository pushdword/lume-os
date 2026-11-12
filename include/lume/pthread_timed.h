/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_PTHREAD_TIMED_H
#define LUME_PTHREAD_TIMED_H
#include <pthread.h>
#include <time.h>
#ifdef __cplusplus
extern "C"
{
#endif
    int pthread_mutex_timedlock(pthread_mutex_t *, const struct timespec *);
#ifdef __cplusplus
}
#endif
#endif
