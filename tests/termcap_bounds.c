#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "termcap.h"

#ifdef _WIN32
#define setenv(name, value, replace) _putenv_s(name, value)
#endif

static unsigned char storage[4160];

static int intact(void)
{
    for (unsigned i = 0; i < sizeof(storage); ++i)
        if ((i < 32 || i >= 1056) && storage[i] != 0x5a)
            return 0;
    return memchr(storage + 32, 0, 1024) != NULL;
}

int main(void)
{
    char *buffer = (char *)storage + 32;
    memset(storage, 0x5a, sizeof(storage));
    if (setenv("TERMCAP", "bounds:co#80:li#24:cl=\\E[H\\E[J:", 1) ||
        tgetent(buffer, "bounds") != 1 || !intact())
        return 1;
    if (tgetnum("co") != 80 || tgetnum("li") != 24)
        return 2;
    memset(storage, 0x5a, sizeof(storage));
    if (setenv("TERMCAP", "/nonexistent/termcap-bounds", 1) || tgetent(buffer, "bounds") != -1 ||
        !intact())
        return 3;
    char entry[2200];
    memcpy(entry, "bounds:", 7);
    memset(entry + 7, 'x', sizeof(entry) - 8);
    entry[sizeof(entry) - 1] = 0;
    memset(storage, 0x5a, sizeof(storage));
    if (setenv("TERMCAP", entry, 1) || tgetent(buffer, "bounds") != 1 || !intact())
        return 4;
    puts("TERMCAP_BOUNDS_PASS");
    return 0;
}
