/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_POSIX_ERRNO_H
#define LUME_POSIX_ERRNO_H
#include_next <errno.h>
#ifdef _GNU_SOURCE
#ifdef __cplusplus
extern "C"
{
#endif
    extern char *program_invocation_name;
    extern char *program_invocation_short_name;
#ifdef __cplusplus
}
#endif
#endif
#endif
