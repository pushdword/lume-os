/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_POSIX_TIME_H
#define LUME_POSIX_TIME_H
#include_next <time.h>

#if __POSIX_VISIBLE && !defined(CLOCK_MONOTONIC)
#define CLOCK_MONOTONIC 4
#endif

#endif
