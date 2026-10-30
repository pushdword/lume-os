#include "lume/abi.h"
#include "lume/graphics.h"
#include "lume/tty.h"
#include "lume/syscall_native.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void require(int condition, const char *name)
{
    if (!condition)
    {
        printf("DESKTOP_PROBE_FAIL %s\n", name);
        exit(1);
    }
}

static int tty(unsigned action, struct lume_tty_request *request)
{
    request->version = LUME_TTY_VERSION;
    request->bytes = sizeof(*request);
    return (int)lume_native_call(LUME_SYS_TTY, action, (uintptr_t)request, sizeof(*request));
}

int app_main(void)
{
    struct lume_tty_request terminals[3] = {0};
    for (unsigned i = 0; i < 3; ++i)
    {
        require(!tty(LUME_TTY_CREATE, terminals + i), "create PTY");
        int master = terminals[i].fd, slave = terminals[i].other;
        struct lume_tty_request raw = {.fd = slave};
        require(!tty(LUME_TTY_GET, &raw), "get terminal mode");
        raw.termios.input = raw.termios.output = raw.termios.local = 0;
        require(!tty(LUME_TTY_SET, &raw), "set raw mode");
        unsigned char sent[] = {(unsigned char)('a' + i), 0, 255, 3}, received[4];
        require(lume_native_call(LUME_SYS_WRITE, master, (uintptr_t)sent, 4) == 4, "PTY input");
        require(lume_native_call(LUME_SYS_READ, slave, (uintptr_t)received, 4) == 4 &&
                    !memcmp(sent, received, 4),
                "PTY raw bytes");
        require(lume_native_call(LUME_SYS_WRITE, slave, (uintptr_t)sent, 4) == 4, "PTY output");
        require(lume_native_call(LUME_SYS_READ, master, (uintptr_t)received, 4) == 4 &&
                    !memcmp(sent, received, 4),
                "PTY output isolation");
    }
    for (unsigned i = 0; i < 3; ++i)
    {
        require(!lume_native_call(LUME_SYS_CLOSE, terminals[i].other, 0, 0), "close slave");
        char path[32];
        struct lume_tty_request name = {.fd = terminals[i].fd};
        require(!tty(LUME_TTY_NAME, &name), "PTY name");
        snprintf(path, sizeof(path), "/dev/pts/%u", name.value);
        int slave = (int)lume_native_call(LUME_SYS_OPEN, (uintptr_t)path, LUME_O_RDWR, 0);
        require(slave >= 0, "reopen slave");
        require(!lume_native_call(LUME_SYS_CLOSE, slave, 0, 0), "close reopened slave");
        require(!lume_native_call(LUME_SYS_CLOSE, terminals[i].fd, 0, 0), "close master");
    }
    struct lume_graphics_request graphics = {.version = LUME_GRAPHICS_VERSION,
                                             .bytes = sizeof(graphics)};
    require(!lume_native_call(LUME_SYS_GRAPHICS, LUME_GRAPHICS_ACQUIRE, (uintptr_t)&graphics,
                              sizeof(graphics)),
            "acquire graphics");
    uint32_t *pixels = malloc((size_t)graphics.pitch * graphics.height);
    require(pixels != NULL, "allocate pixels");
    for (unsigned y = 0; y < graphics.height; ++y)
        for (unsigned x = 0; x < graphics.width; ++x)
            pixels[(size_t)y * graphics.width + x] = (x * 255 / graphics.width) << 16 |
                                                     (y * 255 / graphics.height) << 8 |
                                                     ((x / 32 ^ y / 32) & 1 ? 180 : 40);
    graphics.pixels = (uintptr_t)pixels;
    require(!lume_native_call(LUME_SYS_GRAPHICS, LUME_GRAPHICS_PRESENT, (uintptr_t)&graphics,
                              sizeof(graphics)),
            "present pixels");
    printf("DESKTOP_PROBE_READY %ux%u\n", graphics.width, graphics.height);
    fflush(stdout);
    unsigned keys = 0, motions = 0;
    for (unsigned i = 0; i < 50; ++i)
    {
        struct lume_input_event events[32];
        int64_t bytes =
            lume_native_call(LUME_SYS_READ, graphics.fd, (uintptr_t)events, sizeof(events));
        if (bytes > 0)
            for (unsigned n = 0; n < (unsigned)bytes / sizeof(events[0]); ++n)
            {
                keys += events[n].type == LUME_INPUT_KEY;
                motions += events[n].type == LUME_INPUT_POINTER;
            }
        require(bytes >= 0 || bytes == -LUME_EAGAIN, "input events");
        lume_native_call(LUME_SYS_SLEEP, 100, 0, 0);
    }
    require(!lume_native_call(LUME_SYS_CLOSE, graphics.fd, 0, 0), "release graphics");
    free(pixels);
    printf("DESKTOP_PROBE_PASS ptys=3 keys=%u pointers=%u\n", keys, motions);
    return 0;
}
