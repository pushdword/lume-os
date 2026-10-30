/* SPDX-License-Identifier: BSD-3-Clause */
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

extern int vim_main(int, char **);
extern char *__real_setlocale(int, const char *);

char *__wrap_setlocale(int category, const char *name)
{
    if (name && !*name)
    {
        const char *value = getenv("LC_ALL");
        if (!value || !*value)
            value = getenv(category == LC_NUMERIC    ? "LC_NUMERIC"
                           : category == LC_TIME     ? "LC_TIME"
                           : category == LC_COLLATE  ? "LC_COLLATE"
                           : category == LC_MONETARY ? "LC_MONETARY"
                                                     : "LC_CTYPE");
        if (!value || !*value)
            value = getenv("LANG");
        name = value && *value ? value : "C";
    }
    return __real_setlocale(category, name);
}

int main(int argc, char **argv)
{
    if (!getenv("LANG"))
        setenv("LANG", "C.UTF-8", 0);
    (void)setlocale(LC_ALL, "");
    return vim_main(argc, argv);
}

void term_set_winsize(int height, int width)
{
    if (height > 0 && width > 0)
        printf("\033[8;%d;%dt", height, width);
}
