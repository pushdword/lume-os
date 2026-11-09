#include <stddef.h>
#include <stdint.h>
#include <limits.h>

int memcmp(const void *a, const void *b, size_t count)
{
    const unsigned char *x = a, *y = b;
    for (size_t i = 0; i < count; ++i)
        if (x[i] != y[i])
            return x[i] < y[i] ? -1 : 1;
    return 0;
}

size_t strlen(const char *s)
{
    size_t n = 0;
    while (s[n])
        ++n;
    return n;
}

char *strchr(const char *s, int c)
{
    for (;; ++s)
    {
        if ((unsigned char)*s == (unsigned char)c)
            return (char *)s;
        if (!*s)
            return NULL;
    }
}

int strcmp(const char *a, const char *b)
{
    while (*a && *a == *b)
    {
        ++a;
        ++b;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n)
{
    while (n && *a && *a == *b)
    {
        ++a;
        ++b;
        --n;
    }
    return n ? (unsigned char)*a - (unsigned char)*b : 0;
}

long strtol(const char *s, char **end, int base)
{
    const char *start = s;
    while (*s == ' ' || (*s >= 9 && *s <= 13))
        ++s;
    int negative = *s == '-';
    if (*s == '-' || *s == '+')
        ++s;
    if ((base == 0 || base == 16) && s[0] == '0' && (s[1] == 'x' || s[1] == 'X') &&
        ((s[2] >= '0' && s[2] <= '9') || (s[2] >= 'a' && s[2] <= 'f') ||
         (s[2] >= 'A' && s[2] <= 'F')))
    {
        base = 16;
        s += 2;
    }
    if (!base)
        base = *s == '0' ? 8 : 10;
    if (base < 2 || base > 36)
    {
        if (end)
            *end = (char *)start;
        return 0;
    }
    unsigned long v = 0, limit = (unsigned long)LONG_MAX + (unsigned)negative;
    int any = 0, overflow = 0;
    for (;; ++s)
    {
        unsigned d = *s >= '0' && *s <= '9'   ? (unsigned)(*s - '0')
                     : *s >= 'a' && *s <= 'z' ? (unsigned)(*s - 'a' + 10)
                     : *s >= 'A' && *s <= 'Z' ? (unsigned)(*s - 'A' + 10)
                                              : 36;
        if (d >= (unsigned)base)
            break;
        any = 1;
        if (v > (limit - d) / (unsigned)base)
            overflow = 1;
        else if (!overflow)
            v = v * (unsigned)base + d;
    }
    if (end)
        *end = (char *)(any ? s : start);
    if (overflow)
        return negative ? LONG_MIN : LONG_MAX;
    return negative ? (v == (unsigned long)LONG_MAX + 1 ? LONG_MIN : -(long)v) : (long)v;
}
