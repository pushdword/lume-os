/* SPDX-License-Identifier: BSD-3-Clause */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void *(*volatile copy_bytes)(void *, const void *, size_t) = memcpy;
static void *(*volatile fill_bytes)(void *, int, size_t) = memset;

static uint64_t stamp(void)
{
    struct timespec value;
    if (clock_gettime(CLOCK_MONOTONIC, &value))
        return 0;
    return (uint64_t)value.tv_sec * 1000000000 + (unsigned long)value.tv_nsec;
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    const size_t bytes = 4 * 1024 * 1024;
    unsigned char *source = malloc(bytes + 65536), *target = malloc(bytes + 65536);
    if (!source || !target)
        return 1;
    printf("LAYOUT,source,%llx,target,%llx\n", (unsigned long long)(uintptr_t)source,
           (unsigned long long)(uintptr_t)target);
    for (size_t i = 0; i < bytes + 65536; ++i)
        source[i] = (unsigned char)(i * 17 + i / 4096);
    const unsigned offsets[] = {0, 16, 32, 48, 64, 4096, 8192, 12288, 16384, 20480, 24576, 28672};
    for (unsigned item = 0; item < sizeof(offsets) / sizeof(offsets[0]); ++item)
    {
        unsigned char *out = target + offsets[item];
        uint64_t began = stamp();
        for (unsigned i = 0; i < 256; ++i)
        {
            source[0] = (unsigned char)i;
            copy_bytes(out, source, bytes);
        }
        uint64_t elapsed = stamp() - began;
        if (memcmp(out, source, bytes))
            return 2;
        printf("LAYOUT,copy,%u,%llu\n", offsets[item], (unsigned long long)elapsed);
    }
    for (unsigned item = 0; item < 4; ++item)
    {
        unsigned char *out = target + 16 * item;
        uint64_t began = stamp();
        for (unsigned i = 0; i < 256; ++i)
            fill_bytes(out, (int)(i & 255), bytes);
        uint64_t elapsed = stamp() - began;
        for (size_t i = 0; i < bytes; ++i)
            if (out[i] != 255)
                return 3;
        printf("LAYOUT,fill,%u,%llu\n", 16 * item, (unsigned long long)elapsed);
    }
    free(source);
    free(target);
    puts("LAYOUT_DONE,errors,0");
    return 0;
}
