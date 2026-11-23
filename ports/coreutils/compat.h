/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_COREUTILS_COMPAT_H
#define LUME_COREUTILS_COMPAT_H

#include <config.h>

#if !defined(HAVE_LINKAT) && __has_include(<posix/unistd.h>)
/* The Lume POSIX header declares the SDK's directory-relative hard link API. */
#define HAVE_LINKAT 1
#endif

/* Let replacement sources select when to enable the gnulib stdio wrappers. */
#define _GL_SKIP_GNULIB_STDIO_H
#include <stdio.h>
#include <time.h>
#undef _GL_SKIP_GNULIB_STDIO_H

FILE *lume_coreutils_freopen(const char *path, const char *mode, FILE *stream);
#define freopen lume_coreutils_freopen

#if defined(_PICOLIBC__)
#undef putchar_unlocked
#define putchar_unlocked(character) fputc_unlocked((character), stdout)
#endif

#endif
