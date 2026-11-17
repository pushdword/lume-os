/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_COREUTILS_COMPAT_H
#define LUME_COREUTILS_COMPAT_H

#include <config.h>
#include <stdio.h>
#include <time.h>

#if defined(_PICOLIBC__)
#undef putchar_unlocked
#define putchar_unlocked(character) fputc_unlocked((character), stdout)
#endif

#endif
