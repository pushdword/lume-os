/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_POSIX_LIMITS_H
#define LUME_POSIX_LIMITS_H
#include_next <limits.h>
#ifndef SSIZE_MAX
#define SSIZE_MAX __PTRDIFF_MAX__
#endif
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
#define NGROUPS_MAX 8
#include <lume/files.h>
#undef PATH_MAX
#undef NAME_MAX
#define PATH_MAX LUME_FILES_PATH_MAX
#define NAME_MAX LUME_FILES_NAME_MAX
#endif
#endif
