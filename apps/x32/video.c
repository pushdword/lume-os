/* Lume video control, distinct from monitor EDID discovery (not wired). */
#include "api.h"
#include <string.h>
static unsigned char font[4096];

int app_main(int argc, char **argv)
{
    int modes = lume_syscall(LUME_VIDEO, 0, 0, 0), r = 0;
    if (argc == 1)
    {
        app_puts("Controller modes: ");
        if (modes & 1)
            app_puts("800x600 ");
        if (modes & 2)
            app_puts("1024x768 ");
        app_puts("\nEDID/DDC: not connected; monitor modes unknown\n");
        unsigned geometry = (unsigned)lume_syscall(LUME_TERMINAL, 0, 0, 0);
        app_puts("Text cells: ");
        app_number(geometry & 65535);
        app_puts("x");
        app_number(geometry >> 16);
        app_puts("\nvideo 800x600 | video 1024x768 | video font /path/font.fnt | video measure\n");
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "measure"))
    {
        if (!modes)
        {
            app_error("video", -95);
            return 1;
        }
        unsigned first = (unsigned)lume_syscall(LUME_VIDEO, 3, 0, 0),
                 start = (unsigned)lume_syscall(LUME_INFO, LUME_UPTIME, 0, 0), elapsed;
        do
        {
            lume_syscall(LUME_YIELD, 0, 0, 0);
            elapsed = (unsigned)lume_syscall(LUME_INFO, LUME_UPTIME, 0, 0) - start;
        } while (elapsed < 1000);
        unsigned frames = (unsigned)lume_syscall(LUME_VIDEO, 3, 0, 0) - first,
                 pixels = (unsigned)lume_syscall(LUME_VIDEO, 4, 0, 0);
        app_puts("Pixels: ");
        app_number(pixels & 65535);
        app_puts("x");
        app_number(pixels >> 16);
        app_puts("; frames=");
        app_number(frames);
        app_puts(" elapsed_ms=");
        app_number(elapsed);
        app_puts((lume_syscall(LUME_VIDEO, 5, 0, 0) & 1) ? " PLL locked\n" : " PLL unlocked\n");
        return 0;
    }
    if (argc == 2)
    {
        if (!strcmp(argv[1], "800x600"))
            r = lume_syscall(LUME_VIDEO, 1, 0, 0);
        else if (!strcmp(argv[1], "1024x768"))
            r = lume_syscall(LUME_VIDEO, 1, 1, 0);
        else
            r = -22;
    }
    else if (argc == 3 && !strcmp(argv[1], "font"))
    {
        int fd = app_open(argv[2], LUME_RDONLY);
        if (fd < 0)
            r = fd;
        else
        {
            int n = app_read(fd, font, sizeof font);
            unsigned char extra;
            int tail = n == sizeof font ? app_read(fd, &extra, 1) : -1;
            int closed = app_close(fd);
            r = n == sizeof font && tail == 0
                    ? (closed < 0 ? closed
                                  : lume_syscall(LUME_VIDEO, 2, (uint32_t)(uintptr_t)font, 0))
                    : -22;
        }
    }
    else
        r = -22;
    if (r < 0)
    {
        app_error("video", r);
        return 1;
    }
    app_puts("Video updated\n");
    return 0;
}
