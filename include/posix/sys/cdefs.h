/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_POSIX_SYS_CDEFS_H
#define LUME_POSIX_SYS_CDEFS_H
#include_next <sys/cdefs.h>

#ifndef __BEGIN_DECLS
#ifdef __cplusplus
#define __BEGIN_DECLS                                                                              \
    extern "C"                                                                                     \
    {
#define __END_DECLS }
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif
#endif
#endif
