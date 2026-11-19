/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_COREUTILS_COMPAT_H
#define LUME_COREUTILS_COMPAT_H

#include <config.h>

/* Let replacement sources select when to enable the gnulib stdio wrappers. */
#define _GL_SKIP_GNULIB_STDIO_H
#include <stdio.h>
#include <time.h>
#undef _GL_SKIP_GNULIB_STDIO_H

#if defined(_PICOLIBC__)
#undef putchar_unlocked
#define putchar_unlocked(character) fputc_unlocked((character), stdout)
#endif

#endif
