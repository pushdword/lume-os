#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "api.h"
#include "lume/video_control.h"
#include "../../apps/diagnostics/video.c"

static char output[8192];
static unsigned output_bytes, selected_font, selected_mode, updates, read_position, file_bytes;
static unsigned opened, closed;
static int last_error, query_error;
static const unsigned test_modes[] = {800 | 600u << 16, 1024 | 768u << 16, 1280 | 720u << 16};
static const unsigned test_cells[] = {8 | 16u << 16, 8 | 16u << 16, 12 | 20u << 16, 6 | 10u << 16};
static const unsigned test_faces[] = {LUME_VIDEO_LATIN, LUME_VIDEO_BOLD, LUME_VIDEO_LATIN,
                                      LUME_VIDEO_COMPACT};

int32_t lume_syscall(uint32_t number, uint32_t action, uint32_t value, uint32_t reserved)
{
    assert(!reserved);
    if (number == LUME_TERMINAL)
        return 100 | 37 << 16;
    assert(number == LUME_VIDEO);
    if (query_error)
        return query_error;
    switch (action)
    {
    case LUME_VIDEO_MODE_COUNT:
        return 3;
    case LUME_VIDEO_MODE_PIXELS:
        assert(value < 3);
        return (int32_t)test_modes[value];
    case LUME_VIDEO_MODE_SELECT:
        assert(value < 3);
        selected_mode = value;
        ++updates;
        return 0;
    case LUME_VIDEO_FONT_COUNT:
        return 4;
    case LUME_VIDEO_FONT_CELL:
        assert(value < 4);
        return (int32_t)test_cells[value];
    case LUME_VIDEO_FONT_FACE:
        assert(value < 4);
        return (int32_t)test_faces[value];
    case LUME_VIDEO_FONT_CURRENT:
        return (int32_t)selected_font;
    case LUME_VIDEO_FONT_SELECT:
        assert(value < 4);
        selected_font = value;
        ++updates;
        return 0;
    case LUME_VIDEO_PIXELS:
        return (int32_t)test_modes[selected_mode];
    case LUME_VIDEO_DEPTH:
        return 16;
    case LUME_VIDEO_FRAMES:
    case LUME_VIDEO_STATUS:
        return -38;
    case LUME_VIDEO_UPLOAD:
        assert(value == (uint32_t)(uintptr_t)font && read_position == 4096);
        for (unsigned i = 0; i < sizeof(font); ++i)
            assert(font[i] == (unsigned char)i);
        ++updates;
        return 0;
    default:
        abort();
    }
}

void app_puts(const char *text)
{
    unsigned bytes = (unsigned)strlen(text);
    assert(output_bytes + bytes < sizeof(output));
    memcpy(output + output_bytes, text, bytes + 1);
    output_bytes += bytes;
}

void app_number(uint32_t value)
{
    char text[16];
    snprintf(text, sizeof(text), "%u", value);
    app_puts(text);
}

void app_error(const char *name, int error)
{
    assert(!strcmp(name, "video"));
    last_error = error;
}

int app_open(const char *name, unsigned flags)
{
    assert(!strcmp(name, "/font.fnt") && flags == LUME_RDONLY);
    ++opened;
    read_position = 0;
    return 7;
}

int app_read(int fd, void *data, unsigned bytes)
{
    assert(fd == 7);
    unsigned n = bytes < 257 ? bytes : 257;
    if (n > file_bytes - read_position)
        n = file_bytes - read_position;
    for (unsigned i = 0; i < n; ++i)
        ((unsigned char *)data)[i] = (unsigned char)(read_position + i);
    read_position += n;
    return (int)n;
}

int app_close(int fd)
{
    assert(fd == 7);
    ++closed;
    return 0;
}

static int run(unsigned argc, const char *const *argv)
{
    output_bytes = 0;
    output[0] = 0;
    last_error = 0;
    return app_main((int)argc, (char **)argv);
}

int main(void)
{
    const char *list[] = {"video"};
    assert(!run(1, list) && strstr(output, "1280x720") && strstr(output, "* normal 8x16"));
    const char *mode[] = {"video", "1280x720"};
    assert(!run(2, mode) && selected_mode == 2);
    unsigned before = updates;
    const char *bad[] = {"video", "1920x1080"};
    assert(run(2, bad) == 1 && last_error == -95 && updates == before);
    bad[1] = "65536x768";
    assert(run(2, bad) == 1 && last_error == -22 && updates == before);
    bad[1] = "1024x768junk";
    assert(run(2, bad) == 1 && last_error == -22 && updates == before);
    const char *face[] = {"video", "font", "bold", "8x16"};
    assert(!run(4, face) && selected_font == 1);
    const char *size[] = {"video", "size", "12x20"};
    before = updates;
    assert(run(3, size) == 1 && last_error == -95 && updates == before);
    face[2] = "normal";
    face[3] = "12x20";
    assert(!run(4, face) && selected_font == 2);
    size[2] = "8x16";
    assert(!run(3, size) && selected_font == 0);
    face[2] = "compact";
    assert(!run(3, face) && selected_font == 3);
    const char *file[] = {"video", "font", "/font.fnt"};
    file_bytes = 4096;
    assert(!run(3, file));
    before = updates;
    file_bytes = 4095;
    assert(run(3, file) == 1 && last_error == -22 && updates == before);
    file_bytes = 4097;
    assert(run(3, file) == 1 && last_error == -22 && updates == before);
    assert(opened == closed && closed == 3);
    const char *measure[] = {"video", "measure"};
    assert(run(2, measure) == 1 && last_error == -95);
    query_error = -38;
    assert(run(1, list) == 1 && last_error == -38);
    puts("PASS video command enumeration, mode and font selection, parsing and short file reads");
}
