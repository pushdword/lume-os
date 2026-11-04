// SPDX-License-Identifier: BSD-3-Clause
#ifndef LUME_ICEWM_UTF8FONT_H
#define LUME_ICEWM_UTF8FONT_H
#include "utf8decode.h"

class LumeCoreText
{
    XChar2b *glyphs;
    int count;

  public:
    LumeCoreText(const char *text, int bytes) : glyphs(nullptr), count(0)
    {
        if (!text || bytes <= 0)
            return;
        glyphs = new XChar2b[(size_t)bytes + 3];
        for (size_t offset = 0; offset < (size_t)bytes;)
        {
            unsigned value;
            offset += lume_core_utf8((const unsigned char *)text + offset, (size_t)bytes - offset,
                                     &value);
            if (value > 0xffff || (value >= 0xd800 && value <= 0xdfff))
                value = '?';
            glyphs[count++] = {(unsigned char)(value >> 8), (unsigned char)value};
        }
    }

    ~LumeCoreText()
    {
        delete[] glyphs;
    }

    int width(XFontStruct *font) const
    {
        return count ? XTextWidth16(font, glyphs, count) : 0;
    }

    void draw(XFontStruct *font, Graphics &graphics, int x, int y, int limit, bool ellipsis)
    {
        if (!count)
            return;
        if (limit > 0 && width(font) > limit)
        {
            XChar2b dots[3] = {{0, '.'}, {0, '.'}, {0, '.'}};
            int suffix = ellipsis ? XTextWidth16(font, dots, 3) : 0;
            int low = 0, high = count;
            while (low < high)
            {
                int middle = (low + high + 1) / 2;
                if (XTextWidth16(font, glyphs, middle) + suffix <= limit)
                    low = middle;
                else
                    high = middle - 1;
            }
            count = low;
            if (ellipsis && suffix <= limit)
                for (unsigned i = 0; i < 3; ++i)
                    glyphs[count++] = dots[i];
        }
        XDrawString16(xapp->display(), graphics.drawable(), graphics.handleX(),
                      x - graphics.xorigin(), y - graphics.yorigin(), glyphs, count);
    }
};
#endif
