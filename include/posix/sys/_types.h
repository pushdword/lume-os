/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_POSIX_INTERNAL_TYPES_H
#define LUME_POSIX_INTERNAL_TYPES_H
#include_next <sys/_types.h>

#if defined(_PICOLIBC__)
/* Internal bounds-checking types do not depend on public feature macros. */
typedef size_t __rsize_t;
typedef int __errno_t;
#endif
#endif
