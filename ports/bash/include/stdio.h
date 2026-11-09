#ifndef LUME_BASH_STDIO_H
#define LUME_BASH_STDIO_H
#include_next <stdio.h>
#undef putchar_unlocked
#define putchar_unlocked(c) putc_unlocked((c), stdout)
#undef P_tmpdir
#define P_tmpdir "/data/tmp"
#endif
