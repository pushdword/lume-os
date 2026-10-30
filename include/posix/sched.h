/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_POSIX_SCHED_H
#define LUME_POSIX_SCHED_H
#include_next <sched.h>
#ifdef __cplusplus
extern "C"
{
#endif
    int sched_yield(void);
#ifdef __cplusplus
}
#endif
#endif
