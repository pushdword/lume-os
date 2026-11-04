/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_POSIX_STDIO_H
#define LUME_POSIX_STDIO_H
#include_next <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif
FILE *popen(const char *, const char *);
int pclose(FILE *);
FILE *open_memstream(char **, size_t *);
#ifdef __cplusplus
}
#endif
#endif
