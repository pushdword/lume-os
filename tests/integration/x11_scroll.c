/* SPDX-License-Identifier: BSD-3-Clause */
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <dirent.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lume/abi.h"
#include "lume/syscall_native.h"
#include "perf.h"

static Display *connect_display(void)
{
    DIR *directory = opendir("/data/tmp");
    if (!directory)
        return NULL;
    Display *display = NULL;
    struct dirent *entry;
    while ((entry = readdir(directory)))
    {
        if (strncmp(entry->d_name, "xauth-", 6))
            continue;
        char path[320];
        snprintf(path, sizeof(path), "/data/tmp/%s", entry->d_name);
        setenv("XAUTHORITY", path, 1);
        display = XOpenDisplay("unix/:0");
        if (display)
            break;
    }
    closedir(directory);
    return display;
}

static Window fixture_window(Display *display)
{
    Atom clients = XInternAtom(display, "_NET_CLIENT_LIST", False);
    Atom title = XInternAtom(display, "_NET_WM_NAME", False);
    Atom actual;
    int format;
    unsigned long count, remaining;
    unsigned char *data = NULL;
    if (XGetWindowProperty(display, DefaultRootWindow(display), clients, 0, 256, False, XA_WINDOW,
                           &actual, &format, &count, &remaining, &data) != Success ||
        !data || format != 32)
    {
        if (data)
            XFree(data);
        return None;
    }
    Window result = None;
    for (unsigned long i = 0; i < count && !result; ++i)
    {
        unsigned char *name = NULL;
        unsigned long bytes, rest;
        Window window = ((Window *)data)[i];
        if (XGetWindowProperty(display, window, title, 0, 128, False, AnyPropertyType, &actual,
                               &format, &bytes, &rest, &name) == Success &&
            name && format == 8 && bytes >= 20 && !memcmp(name, "Lume timed scroll y=", 20))
            result = window;
        if (name)
            XFree(name);
    }
    XFree(data);
    return result;
}

static long scroll_position(Display *display, Window window)
{
    Atom actual;
    int format;
    unsigned long bytes, rest;
    unsigned char *name = NULL;
    long position = -1;
    Atom title = XInternAtom(display, "_NET_WM_NAME", False);
    if (XGetWindowProperty(display, window, title, 0, 128, False, AnyPropertyType, &actual, &format,
                           &bytes, &rest, &name) == Success &&
        name && format == 8 && bytes >= 20 && !memcmp(name, "Lume timed scroll y=", 20))
        position = strtol((char *)name + 20, NULL, 10);
    if (name)
        XFree(name);
    return position;
}

static uint64_t milliseconds(void)
{
    return (uint64_t)lume_native_call(LUME_SYS_CLOCK, 0, 0, 0);
}

static int snapshot(struct lume_perf_snapshot *out)
{
    return (int)lume_native_call(LUME_SYS_TELEMETRY, (uintptr_t)out, sizeof(*out),
                                 LUME_PERF_VERSION);
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    Display *display = connect_display();
    if (!display)
        return 1;
    Window window = None;
    for (unsigned i = 0; i < 100 && !window; ++i)
    {
        window = fixture_window(display);
        if (!window)
            poll(NULL, 0, 100);
    }
    long initial = window ? scroll_position(display, window) : -1;
    for (unsigned i = 0; window && initial <= 0 && i < 100; ++i)
    {
        poll(NULL, 0, 100);
        initial = scroll_position(display, window);
    }
    if (!window || initial <= 0)
    {
        XCloseDisplay(display);
        fputs("SCROLL_ERROR timed fixture did not start scrolling\n", stderr);
        return 2;
    }
    struct lume_perf_snapshot previous, current;
    if (snapshot(&previous))
        return 4;
    uint64_t start = milliseconds(), next_sample = start + 1000;
    printf("SCROLL_BEGIN window=%lu uptime_ms=%llu duration_ms=20000 mode=page-script hz=%u\n",
           window, (unsigned long long)start, previous.clock_hz);
    while (milliseconds() - start < 20000)
    {
        uint64_t now = milliseconds();
        if (now >= next_sample)
        {
            if (snapshot(&current))
                return 6;
            printf("SCROLL_SAMPLE ms=%llu y=%ld", (unsigned long long)(now - start),
                   scroll_position(display, window));
            for (unsigned i = 0; i < current.core_count; ++i)
                printf(" c%u=%u,%u,%u,%u", i,
                       current.cores[i].ticks[LUME_PERF_USER] -
                           previous.cores[i].ticks[LUME_PERF_USER],
                       current.cores[i].ticks[LUME_PERF_KERNEL] -
                           previous.cores[i].ticks[LUME_PERF_KERNEL],
                       current.cores[i].ticks[LUME_PERF_LOCKWAIT] -
                           previous.cores[i].ticks[LUME_PERF_LOCKWAIT],
                       current.cores[i].ticks[LUME_PERF_IDLE] -
                           previous.cores[i].ticks[LUME_PERF_IDLE]);
            putchar('\n');
            previous = current;
            next_sample = now + 1000;
        }
        poll(NULL, 0, 2);
    }
    XSync(display, False);
    long position = scroll_position(display, window);
    XCloseDisplay(display);
    printf("SCROLL_DONE start_y=%ld y=%ld\n", initial, position);
    return position - initial >= 1000 ? 0 : 7;
}
