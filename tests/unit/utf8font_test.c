/* SPDX-License-Identifier: BSD-3-Clause */
#include "../../ports/icewm/utf8decode.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    const unsigned char text[] = "ação café €";
    unsigned expected[] = {'a', 0xe7, 0xe3, 'o', ' ', 'c', 'a', 'f', 0xe9, ' ', 0x20ac};
    size_t offset = 0;
    for (unsigned i = 0; i < sizeof(expected) / sizeof(expected[0]); ++i)
    {
        unsigned point;
        size_t bytes = lume_core_utf8(text + offset, sizeof(text) - 1 - offset, &point);
        assert(bytes && point == expected[i]);
        offset += bytes;
    }
    assert(offset == sizeof(text) - 1);
    const unsigned char *invalid[] = {(const unsigned char *)"\xc0\xaf",
                                      (const unsigned char *)"\xed\xa0\x80",
                                      (const unsigned char *)"\xf4\x90\x80\x80",
                                      (const unsigned char *)"\xe2\x82",
                                      (const unsigned char *)"\x80",
                                      (const unsigned char *)"\xe0\x80\xaf"};
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
    {
        unsigned point;
        assert(lume_core_utf8(invalid[i], strlen((const char *)invalid[i]), &point) == 1 &&
               point == '?');
    }
    unsigned point;
    assert(lume_core_utf8((const unsigned char *)"\xf0\x9f\x98\x80", 4, &point) == 4 &&
           point == 0x1f600);
    assert(!lume_core_utf8(text, 0, &point));
    puts("CORE_FONT_UTF8_PASS");
}
