/* SPDX-License-Identifier: BSD-3-Clause */
#include "termcap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define TBUFSZ 1024
#define ESC 27
#define Ctrl_chr(value) ((value) & 31)
#define char_u unsigned char
#define mch_getenv(name) getenv((const char *)(name))
#define mch_fopen fopen
#define VIM_ISDIGIT(value) isdigit((unsigned char)(value))
#define STRMOVE(target, source) memmove(target, source, strlen(source) + 1)

static inline void vim_strncpy(char *target, const char *source, size_t bytes)
{
    strncpy(target, source, bytes);
    target[bytes] = 0;
}
