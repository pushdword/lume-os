/* SPDX-License-Identifier: BSD-3-Clause */
#include <assert.h>
#include <langinfo.h>
#include <locale.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int format(char *buffer, size_t length, const char *pattern, ...)
{
    va_list arguments;
    va_start(arguments, pattern);
    int result = vsnprintf(buffer, length, pattern, arguments);
    va_end(arguments);
    return result;
}

int main(void)
{
    assert(setlocale(LC_CTYPE, "C"));
    assert(!strcmp(nl_langinfo(CODESET), "ASCII"));
    assert(setlocale(LC_CTYPE, "pt_PT.UTF-8"));
    assert(!strcmp(nl_langinfo(CODESET), "UTF-8"));
    assert(setlocale(LC_CTYPE, "C"));
    puts("LIBC_LOCALE_CODESET_PASS");
    char buffer[128];
    memset(buffer, 0x55, sizeof(buffer));
    assert(snprintf(buffer, 5, "1234567") == 7);
    assert(!strcmp(buffer, "1234") && buffer[5] == 0x55);
    assert(snprintf(buffer, 0, "1234567") == 7 && !strcmp(buffer, "1234"));
    assert(snprintf(NULL, 0, "1234567") == 7);
    assert(format(buffer, 5, "%s", "1234567") == 7 && !strcmp(buffer, "1234"));
    assert(format(buffer, 0, "%s", "1234567") == 7);
    assert(format(NULL, 0, "%s", "1234567") == 7);
    puts("PRINTF_C99_PASS");
    assert(snprintf(buffer, sizeof(buffer), "%2$d %3$d %1$d", 1, 2, 3) == 5);
    assert(!strcmp(buffer, "2 3 1"));
    assert(format(buffer, sizeof(buffer), "%2$*1$d", 5, 42) == 5);
    assert(!strcmp(buffer, "   42"));
    assert(format(buffer, sizeof(buffer), "%2$.*1$s", 3, "abcdef") == 3);
    assert(!strcmp(buffer, "abc"));
    assert(format(buffer, sizeof(buffer), "%2$s %1$llu", UINT64_C(0x100000001), "valor") == 16);
    assert(!strcmp(buffer, "valor 4294967297"));
    assert(format(buffer, sizeof(buffer), "%2$.2f %1$d", 7, 1.25) == 6);
    assert(!strcmp(buffer, "1.25 7"));
    puts("PRINTF_POSITIONAL_PASS");
    puts("PRINTF_NATIVE_PASS");
    return 0;
}
