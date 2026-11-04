/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_CORE_UTF8_DECODE_H
#define LUME_CORE_UTF8_DECODE_H
#include <stddef.h>

static inline size_t lume_core_utf8(const unsigned char *text, size_t bytes, unsigned *point)
{
    *point = '?';
    if (!bytes)
        return 0;
    if (text[0] < 0x80)
    {
        *point = text[0];
        return 1;
    }
    unsigned count = text[0] >= 0xc2 && text[0] <= 0xdf   ? 2
                     : text[0] >= 0xe0 && text[0] <= 0xef ? 3
                     : text[0] >= 0xf0 && text[0] <= 0xf4 ? 4
                                                          : 0;
    if (!count || count > bytes)
        return 1;
    unsigned value = text[0] & (0x7fu >> count);
    for (unsigned i = 1; i < count; ++i)
    {
        if ((text[i] & 0xc0) != 0x80)
            return 1;
        value = (value << 6) | (text[i] & 0x3f);
    }
    unsigned minimum = count == 2 ? 0x80 : count == 3 ? 0x800 : 0x10000;
    if (value < minimum || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff))
        return 1;
    *point = value;
    return count;
}
#endif
