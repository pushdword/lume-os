/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_POSIX_STDIO_H
#define LUME_POSIX_STDIO_H
#define fflush __picolibc_fflush_declaration
#include_next <stdio.h>
#undef fflush
#ifdef __cplusplus
extern "C"
{
#endif
    FILE *popen(const char *, const char *);
    int pclose(FILE *);
    FILE *open_memstream(char **, size_t *);
    int renameat(int, const char *, int, const char *);
    int fflush(FILE *);
#ifdef __cplusplus
}
#endif
#endif
