#include <stddef.h>
#include <stdint.h>

typedef uint32_t scan_word __attribute__((__may_alias__));

char *strchr(const char *text, int value)
{
    const unsigned char *p = (const unsigned char *)text;
    unsigned char target = (unsigned char)value;
    while ((uintptr_t)p & 3u)
    {
        if (*p == target)
            return (char *)(uintptr_t)p;
        if (!*p)
            return NULL;
        ++p;
    }
    const uint32_t ones = 0x01010101u, highs = 0x80808080u;
    const uint32_t repeated = (uint32_t)target * ones;
    for (;; p += 4)
    {
        /* An aligned word cannot cross a 4 KiB page boundary. */
        uint32_t word = *(const scan_word *)p, match = word ^ repeated;
        if (((word - ones) & ~word & highs) || ((match - ones) & ~match & highs))
            for (unsigned i = 0; i < 4; ++i)
            {
                unsigned char byte = (unsigned char)(word >> (i * 8u));
                if (byte == target)
                    return (char *)(uintptr_t)(p + i);
                if (!byte)
                    return NULL;
            }
    }
}

size_t strlen(const char *text)
{
    const unsigned char *p = (const unsigned char *)text;
    while ((uintptr_t)p & 3u)
    {
        if (!*p)
            return (size_t)(p - (const unsigned char *)text);
        ++p;
    }
    for (;; p += 4)
    {
        uint32_t word = *(const scan_word *)p;
        if ((word - 0x01010101u) & ~word & 0x80808080u)
            for (unsigned i = 0; i < 4; ++i)
                if (!(unsigned char)(word >> (i * 8u)))
                    return (size_t)(p + i - (const unsigned char *)text);
    }
}

void *memchr(const void *memory, int value, size_t count)
{
    const unsigned char *p = (const unsigned char *)memory;
    unsigned char target = (unsigned char)value;
    while (count && ((uintptr_t)p & 3u))
    {
        if (*p == target)
            return (void *)(uintptr_t)p;
        ++p;
        --count;
    }
    uint32_t repeated = (uint32_t)target * 0x01010101u;
    while (count >= 4)
    {
        uint32_t word = *(const scan_word *)p, match = word ^ repeated;
        if ((match - 0x01010101u) & ~match & 0x80808080u)
            for (unsigned i = 0; i < 4; ++i)
                if ((unsigned char)(word >> (i * 8u)) == target)
                    return (void *)(uintptr_t)(p + i);
        p += 4;
        count -= 4;
    }
    while (count--)
    {
        if (*p == target)
            return (void *)(uintptr_t)p;
        ++p;
    }
    return NULL;
}
