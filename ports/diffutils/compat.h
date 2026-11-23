#ifndef LUME_DIFFUTILS_COMPAT_H
#define LUME_DIFFUTILS_COMPAT_H
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#include <config.h>
#include <stdio.h>
#undef putchar_unlocked
#define putchar_unlocked(character) fputc_unlocked((character), stdout)
int pclose(FILE *stream);
#define DEFAULT_DIFF_PROGRAM "/data/bin/diff"
#endif
