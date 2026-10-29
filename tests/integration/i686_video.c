#include "api.h"
#include "lume/video_control.h"
#include <stdio.h>
#include <string.h>

static void text(const char *value)
{
    fputs(value, stdout);
}

int app_main(unsigned argc, char **argv)
{
    unsigned pixels = (unsigned)lume_syscall(LUME_VIDEO, 4, 0, 0);
    unsigned geometry = (unsigned)lume_syscall(LUME_TERMINAL, 0, 0, 0);
    unsigned columns = geometry & 65535, rows = geometry >> 16;
    int current_font = lume_syscall(LUME_VIDEO, LUME_VIDEO_FONT_CURRENT, 0, 0);
    unsigned cell =
        (unsigned)lume_syscall(LUME_VIDEO, LUME_VIDEO_FONT_CELL, (unsigned)current_font, 0);
    unsigned expected =
        ((pixels & 65535) - 16) / (cell & 65535) | (((pixels >> 16) - 40) / (cell >> 16)) << 16;
    if (current_font < 0 || !cell || geometry != expected ||
        lume_syscall(LUME_VIDEO, 6, 0, 0) != (int)((pixels & 65535) * 2) ||
        lume_syscall(LUME_VIDEO, 7, 0, 0) != 16 || lume_syscall(LUME_VIDEO, 1, 2, 0) != -22 ||
        lume_syscall(LUME_VIDEO, 1, 0, 1) != -22)
    {
        text("VIDEOTEST FAIL geometry or mode validation\n");
        return 1;
    }
    if (argc == 2 && !strcmp(argv[1], "catalog"))
    {
        int modes = lume_syscall(LUME_VIDEO, LUME_VIDEO_MODE_COUNT, 0, 0);
        int fonts = lume_syscall(LUME_VIDEO, LUME_VIDEO_FONT_COUNT, 0, 0);
        if (modes <= 0 || fonts < 8)
            return 2;
        for (int mode = 0; mode < modes; ++mode)
        {
            if (lume_syscall(LUME_VIDEO, LUME_VIDEO_MODE_SELECT, (unsigned)mode, 0))
                return 3;
            unsigned mp = (unsigned)lume_syscall(LUME_VIDEO, LUME_VIDEO_PIXELS, 0, 0);
            for (int font = 0; font < fonts; ++font)
            {
                if (lume_syscall(LUME_VIDEO, LUME_VIDEO_FONT_SELECT, (unsigned)font, 0))
                    return 4;
                unsigned fc =
                    (unsigned)lume_syscall(LUME_VIDEO, LUME_VIDEO_FONT_CELL, (unsigned)font, 0);
                unsigned actual = (unsigned)lume_syscall(LUME_TERMINAL, 0, 0, 0);
                unsigned wanted =
                    ((mp & 65535) - 16) / (fc & 65535) | (((mp >> 16) - 40) / (fc >> 16)) << 16;
                if (actual != wanted)
                    return 5;
                text("Fonte: a\xc3\xa7\xc3\xa3o, \xe2\x82\xac\n");
                fflush(stdout);
            }
        }
        if (lume_syscall(LUME_VIDEO, LUME_VIDEO_MODE_SELECT, (unsigned)modes, 0) != -22 ||
            lume_syscall(LUME_VIDEO, LUME_VIDEO_FONT_SELECT, (unsigned)fonts, 0) != -22 ||
            lume_syscall(LUME_VIDEO, LUME_VIDEO_UPLOAD, 0, 0) != -14)
            return 6;
        int restored = 0;
        for (int mode = 0; mode < modes; ++mode)
            if ((unsigned)lume_syscall(LUME_VIDEO, LUME_VIDEO_MODE_PIXELS, (unsigned)mode, 0) ==
                pixels)
            {
                restored = !lume_syscall(LUME_VIDEO, LUME_VIDEO_MODE_SELECT, (unsigned)mode, 0);
                break;
            }
        if (!restored ||
            lume_syscall(LUME_VIDEO, LUME_VIDEO_FONT_SELECT, (unsigned)current_font, 0))
            return 7;
        printf("VIDEOTEST CATALOG OK modes=%d fonts=%d\n", modes, fonts);
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "scroll"))
    {
        for (unsigned i = 0; i < rows + 8; ++i)
            printf("Scroll line %02u / %u columns\n", i, columns);
        text("VIDEOTEST SCROLL OK\n");
        return 0;
    }
    text("\033[2J\033[HLume i686 / S3 Trio64V+\n");
    printf("RGB565 %ux%u / console %ux%u\n\n", pixels & 65535, pixels >> 16, columns, rows);
    for (unsigned color = 0; color < 8; ++color)
        printf("\033[%um %u ", 40 + color, color);
    text("\033[0m\n");
    for (unsigned color = 0; color < 8; ++color)
        printf("\033[%um COLOR ", 30 + color);
    text(
        "\033[0m\n\nUTF-8: a\xc3\xa7\xc3\xa3o, \xc3\xb3rg\xc3\xa3o, portugu\xc3\xaas, \xe2\x82\xac\n");
    text("\033[12;1HCURSOR_OLD\033[12;1HCURSOR_OK\033[K\n");
    text("\033[sSaved cursor\033[uRestored cursor\033[K\n");
    text("\033[?25lCursor hidden\033[?25h\n");
    text("VIDEOTEST ANSI UTF8 GEOMETRY OK\n");
    return 0;
}
