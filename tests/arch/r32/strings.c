#define _DEFAULT_SOURCE
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static _Alignas(4096) volatile unsigned char bytes[8192];

int main(void)
{
    static const unsigned lengths[] = {0,   1,   2,   3,   4,   5,   7,   8,   15,
                                       16,  17,  31,  32,  63,  64,  65,  127, 128,
                                       129, 255, 256, 257, 511, 512, 513, 1023};
    unsigned cases = 0;
    for (unsigned l = 0; l < sizeof lengths / sizeof lengths[0]; ++l)
        for (unsigned offset = 0; offset < 8; ++offset)
        {
            unsigned n = lengths[l];
            for (unsigned i = 0; i < n; ++i)
                bytes[offset + i] = (unsigned char)((i * 29u + (i >> 3)) % 255u + 1u);
            bytes[offset + n] = 0;
            const char *text = (const char *)(bytes + offset);
            if (strlen(text) != n)
                return 1;
            ++cases;
            for (int value = -1; value <= 256; ++value)
            {
                const char *want = NULL, *prefix = NULL;
                for (unsigned i = 0; i <= n; ++i)
                    if (bytes[offset + i] == (unsigned char)value)
                    {
                        want = text + i;
                        break;
                    }
                for (unsigned i = 0; i < n / 2; ++i)
                    if (bytes[offset + i] == (unsigned char)value)
                    {
                        prefix = text + i;
                        break;
                    }
                const char *memory = want == text + n ? NULL : want;
                if (strchr(text, value) != want || memchr(text, value, n) != memory ||
                    memchr(text, value, n / 2) != prefix)
                {
                    printf("R32_STRING_WORDS_FAIL n=%u offset=%u value=%d\n", n, offset, value);
                    return 2;
                }
                ++cases;
            }
        }
    void *old = sbrk(0);
    if (old == (void *)-1)
        return 3;
    unsigned extent = 4096u - ((uintptr_t)old & 4095u) + 4096u;
    if (sbrk(extent) != old)
        return 4;
    unsigned char *end = (unsigned char *)old + extent;
    for (unsigned n = 1; n <= 65; ++n)
    {
        for (unsigned i = 0; i < n; ++i)
            end[-(int)n + (int)i] = 'A';
        end[-1] = 0;
        const char *text = (const char *)(end - n);
        if (strlen(text) != n - 1u || strchr(text, 255) || memchr(text, 255, n) ||
            memchr(text, 0, n) != end - 1 || memchr(text, 0, n - 1u))
            return 5;
        ++cases;
    }
    if (sbrk(-(ptrdiff_t)extent) != end)
        return 6;
    printf("R32_STRING_WORDS_PASS cases=%u\n", cases);
    return 0;
}
