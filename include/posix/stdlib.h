/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_POSIX_STDLIB_H
#define LUME_POSIX_STDLIB_H

#include_next <stdlib.h>

#if defined(LUME_NATIVE_ABI)
#ifdef __cplusplus
extern "C"
{
#endif

    const char *getprogname(void);

#ifdef __cplusplus
}
#endif
#endif

#endif
