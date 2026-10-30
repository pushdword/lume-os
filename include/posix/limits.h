/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_POSIX_LIMITS_H
#define LUME_POSIX_LIMITS_H
#include_next <limits.h>
#ifndef SSIZE_MAX
#define SSIZE_MAX __PTRDIFF_MAX__
#endif
#endif
