#include "api.h"
#include "lume/video_control.h"
#include <string.h>

static unsigned char font[4096];
static const char *const faces[] = {"normal", "bold", "compact", "custom"};

static int call(unsigned action, unsigned value)
{
    return lume_syscall(LUME_VIDEO, action, value, 0);
}

static void dimensions(unsigned value)
{
    app_number(value & 65535);
    app_puts("x");
    app_number(value >> 16);
}

static int parse_dimensions(const char *text, unsigned *value)
{
    unsigned width = 0, height = 0, digits = 0;
    while (*text >= '0' && *text <= '9')
    {
        if (++digits > 5)
            return -22;
        width = width * 10 + (unsigned)(*text++ - '0');
    }
    if (!width || width > 65535 || *text++ != 'x')
        return -22;
    digits = 0;
    while (*text >= '0' && *text <= '9')
    {
        if (++digits > 5)
            return -22;
        height = height * 10 + (unsigned)(*text++ - '0');
    }
    if (!height || height > 65535 || *text)
        return -22;
    *value = width | height << 16;
    return 0;
}

static int list_modes(void)
{
    int count = call(LUME_VIDEO_MODE_COUNT, 0);
    if (count < 0)
        return count;
    app_puts("Controller modes:\n");
    for (int i = 0; i < count; ++i)
    {
        int pixels = call(LUME_VIDEO_MODE_PIXELS, (unsigned)i);
        if (pixels < 0)
            return pixels;
        app_puts("  ");
        dimensions((unsigned)pixels);
        app_puts("\n");
    }
    if (!count)
        app_puts("  No switchable pixel modes\n");
    return 0;
}

static int list_fonts(void)
{
    int count = call(LUME_VIDEO_FONT_COUNT, 0);
    if (count < 0)
        return count;
    int selected = call(LUME_VIDEO_FONT_CURRENT, 0);
    app_puts("Console fonts (cell pixels):\n");
    for (int i = 0; i < count; ++i)
    {
        int cell = call(LUME_VIDEO_FONT_CELL, (unsigned)i);
        int face = call(LUME_VIDEO_FONT_FACE, (unsigned)i);
        if (cell < 0 || face < 0 || face >= 4)
            return -5;
        app_puts(i == selected ? "* " : "  ");
        app_puts(faces[face]);
        app_puts(" ");
        dimensions((unsigned)cell);
        app_puts("\n");
    }
    if (!count)
        app_puts("  The controller retains its fixed font\n");
    return 0;
}

static int select_mode(const char *text)
{
    unsigned pixels;
    int result = parse_dimensions(text, &pixels);
    if (result)
        return result;
    int count = call(LUME_VIDEO_MODE_COUNT, 0);
    if (count < 0)
        return count;
    for (int i = 0; i < count; ++i)
        if (call(LUME_VIDEO_MODE_PIXELS, (unsigned)i) == (int)pixels)
            return call(LUME_VIDEO_MODE_SELECT, (unsigned)i);
    return -95;
}

static int face_named(const char *name)
{
    for (unsigned i = 0; i < 4; ++i)
        if (!strcmp(name, faces[i]))
            return (int)i;
    return -1;
}

static int select_font(const char *name, const char *size)
{
    int selected = call(LUME_VIDEO_FONT_CURRENT, 0);
    if (selected < 0)
        return selected;
    int face = call(LUME_VIDEO_FONT_FACE, (unsigned)selected);
    int current_cell = call(LUME_VIDEO_FONT_CELL, (unsigned)selected);
    if (face < 0 || current_cell < 0)
        return -5;
    if (name)
    {
        face = face_named(name);
        if (face < 0)
            return -22;
    }
    unsigned cell = (unsigned)current_cell;
    if (size && parse_dimensions(size, &cell))
        return -22;
    int count = call(LUME_VIDEO_FONT_COUNT, 0), fallback = -1;
    if (count < 0)
        return count;
    for (int i = 0; i < count; ++i)
        if (call(LUME_VIDEO_FONT_FACE, (unsigned)i) == face)
        {
            if (fallback < 0)
                fallback = i;
            if (call(LUME_VIDEO_FONT_CELL, (unsigned)i) == (int)cell)
                return call(LUME_VIDEO_FONT_SELECT, (unsigned)i);
        }
    return !size && fallback >= 0 ? call(LUME_VIDEO_FONT_SELECT, (unsigned)fallback) : -95;
}

static int upload_font(const char *path)
{
    int fd = app_open(path, LUME_RDONLY);
    if (fd < 0)
        return fd;
    unsigned count = 0;
    int result = 0;
    while (count < sizeof(font))
    {
        int n = app_read(fd, font + count, sizeof(font) - count);
        if (n <= 0)
        {
            result = n < 0 ? n : -22;
            break;
        }
        count += (unsigned)n;
    }
    unsigned char extra;
    if (!result)
    {
        int tail = app_read(fd, &extra, 1);
        result = tail < 0 ? tail : tail ? -22 : 0;
    }
    int closed = app_close(fd);
    if (!result && closed < 0)
        result = closed;
    return result ? result
                  : lume_syscall(LUME_VIDEO, LUME_VIDEO_UPLOAD, (uint32_t)(uintptr_t)font, 0);
}

static int measure(void)
{
    int first = call(LUME_VIDEO_FRAMES, 0), status = call(LUME_VIDEO_STATUS, 0);
    if (first < 0 || status < 0)
        return -95;
    unsigned start = (unsigned)lume_syscall(LUME_INFO, LUME_UPTIME, 0, 0), elapsed;
    do
    {
        lume_syscall(LUME_YIELD, 0, 0, 0);
        elapsed = (unsigned)lume_syscall(LUME_INFO, LUME_UPTIME, 0, 0) - start;
    } while (elapsed < 1000);
    int last = call(LUME_VIDEO_FRAMES, 0);
    if (last < 0)
        return last;
    app_puts("Frames: ");
    app_number((unsigned)last - (unsigned)first);
    app_puts(" elapsed_ms=");
    app_number(elapsed);
    app_puts((call(LUME_VIDEO_STATUS, 0) & 1) ? " PLL locked\n" : " PLL unlocked\n");
    return 0;
}

int app_main(int argc, char **argv)
{
    int result;
    if (argc == 1)
    {
        result = list_modes();
        if (!result)
            result = list_fonts();
        if (!result)
        {
            int pixels = call(LUME_VIDEO_PIXELS, 0);
            if (pixels > 0)
            {
                app_puts("Pixels: ");
                dimensions((unsigned)pixels);
                int depth = call(LUME_VIDEO_DEPTH, 0);
                if (depth > 0)
                {
                    app_puts(" bpp=");
                    app_number((unsigned)depth);
                }
                app_puts("\n");
            }
            app_puts("Text cells: ");
            dimensions((unsigned)lume_syscall(LUME_TERMINAL, 0, 0, 0));
            app_puts("\nvideo WIDTHxHEIGHT | video modes | video fonts\n"
                     "video font normal|bold|compact [WIDTHxHEIGHT]\n"
                     "video size WIDTHxHEIGHT | video font /path/font.fnt | video measure\n");
        }
    }
    else if (argc == 2 && !strcmp(argv[1], "modes"))
        result = list_modes();
    else if (argc == 2 && !strcmp(argv[1], "fonts"))
        result = list_fonts();
    else if (argc == 2 && !strcmp(argv[1], "measure"))
        result = measure();
    else if (argc == 2)
        result = select_mode(argv[1]);
    else if (argc == 3 && !strcmp(argv[1], "size"))
        result = select_font(0, argv[2]);
    else if ((argc == 3 || argc == 4) && !strcmp(argv[1], "font"))
        result = argc == 3 && face_named(argv[2]) < 0
                     ? upload_font(argv[2])
                     : select_font(argv[2], argc == 4 ? argv[3] : 0);
    else
        result = -22;
    if (result < 0)
    {
        app_error("video", result);
        return 1;
    }
    if (argc > 2 || (argc == 2 && strcmp(argv[1], "modes") && strcmp(argv[1], "fonts") &&
                     strcmp(argv[1], "measure")))
        app_puts("Video updated\n");
    return 0;
}
