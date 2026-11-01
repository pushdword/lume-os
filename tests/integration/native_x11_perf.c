/* SPDX-License-Identifier: BSD-3-Clause */
#define main x11_contract_main
#include "amd64_x11.c"
#undef main
#include "perf.h"
#include <stdbool.h>
#include <time.h>

static uint64_t milliseconds(void)
{
    struct timespec value;
    if (clock_gettime(CLOCK_MONOTONIC, &value))
        return 0;
    return (uint64_t)value.tv_sec * 1000 + (unsigned)value.tv_nsec / 1000000;
}

static int snapshot(struct lume_perf_snapshot *value)
{
    return lume_native_call(LUME_SYS_TELEMETRY, (uintptr_t)value, sizeof(*value),
                            LUME_PERF_VERSION) != 0;
}

static void motion(Display *display, unsigned opcode, int x, int y)
{
    xXTestFakeInputReq *request = _XGetRequest(display, opcode, sizeof(*request));
    memset((char *)request + sizeof(xReq), 0, sizeof(*request) - sizeof(xReq));
    request->xtReqType = X_XTestFakeInput;
    request->type = MotionNotify;
    request->root = DefaultRootWindow(display);
    request->rootX = x;
    request->rootY = y;
    XSync(display, False);
}

int main(int argc, char **argv)
{
    if (argc != 3 || (strcmp(argv[1], "idle") && strcmp(argv[1], "drag")))
        return 2;
    unsigned seconds = (unsigned)strtoul(argv[2], NULL, 10);
    if (!seconds || seconds > 15)
        return 2;
    bool drag = !strcmp(argv[1], "drag");
    Display *display = drag ? XOpenDisplay(NULL) : NULL;
    int opcode = 0, event, error, origin_x = 0, origin_y = 0;
    Window window = None, child;
    if (drag)
    {
        if (!display || !XQueryExtension(display, "XTEST", &opcode, &event, &error))
            return 1;
        Atom type;
        int format;
        unsigned long count, after;
        unsigned char *list = NULL;
        XGetWindowProperty(display, DefaultRootWindow(display),
                           XInternAtom(display, "_NET_CLIENT_LIST", False), 0, 128, False,
                           XA_WINDOW, &type, &format, &count, &after, &list);
        if (type != XA_WINDOW || format != 32 || !count || !list)
            return 1;
        window = ((Window *)list)[0];
        XFree(list);
        XTranslateCoordinates(display, window, DefaultRootWindow(display), 0, 0, &origin_x,
                              &origin_y, &child);
        fake_input(display, MotionNotify, 0, origin_x + 80, origin_y - 10);
        fake_input(display, ButtonPress, 1, 0, 0);
    }
    struct lume_perf_snapshot before, after;
    if (snapshot(&before))
        return 1;
    uint64_t start = milliseconds(), end = start + seconds * 1000u;
    unsigned frames = 0;
    while (milliseconds() < end)
    {
        if (drag)
        {
            unsigned phase = frames % 120;
            unsigned distance = phase < 60 ? phase : 120 - phase;
            motion(display, (unsigned)opcode, origin_x + 80 + distance * 3,
                   origin_y - 10 + distance);
        }
        ++frames;
        uint64_t target = start + (uint64_t)frames * 1000 / 60;
        uint64_t now = milliseconds();
        if (target > now)
            usleep((target - now) * 1000);
    }
    int final_x = origin_x, final_y = origin_y;
    if (drag)
    {
        motion(display, (unsigned)opcode, origin_x + 200, origin_y + 50);
        fake_input(display, ButtonRelease, 1, 0, 0);
    }
    usleep(200000);
    if (snapshot(&after) || after.core_count != before.core_count)
        return 1;
    if (drag)
    {
        XTranslateCoordinates(display, window, DefaultRootWindow(display), 0, 0, &final_x, &final_y,
                              &child);
        XCloseDisplay(display);
        if (final_x == origin_x && final_y == origin_y)
            return 1;
    }
    printf(
        "X11_PERF mode=%s elapsed_ms=%llu motions=%u clock_hz=%u cores=%u origin=%d,%d final=%d,%d\n",
        argv[1], (unsigned long long)(milliseconds() - start), drag ? frames + 1 : 0,
        after.clock_hz, after.core_count, origin_x, origin_y, final_x, final_y);
    for (unsigned core = 0; core < after.core_count; ++core)
    {
        printf("X11_PERF_CORE index=%u counter=%u", core,
               after.cores[core].counter - before.cores[core].counter);
        for (unsigned kind = 0; kind < LUME_PERF_MODES; ++kind)
            printf(" ticks%u=%u", kind,
                   after.cores[core].ticks[kind] - before.cores[core].ticks[kind]);
        printf("\n");
    }
    puts("X11_PERF_PASS");
    return 0;
}
