/* SPDX-License-Identifier: BSD-3-Clause */
#define _POSIX_C_SOURCE 200809L
#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/xtestproto.h>
#include <X11/keysym.h>
#include <dirent.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <unistd.h>

struct sample
{
    uint64_t start, end;
    long position;
};

static volatile sig_atomic_t interrupted;

static void stop(int number)
{
    interrupted = number;
}

static uint64_t stamp(void)
{
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC, &t))
        exit(1);
    return (uint64_t)t.tv_sec * UINT64_C(1000000000) + (uint64_t)t.tv_nsec;
}

static Display *open_display(bool scan_auth)
{
    if (scan_auth && !getenv("DISPLAY"))
        setenv("DISPLAY", "unix/:0", 1);
    Display *display = XOpenDisplay(NULL);
    if (display || !scan_auth)
        return display;
    DIR *dir = opendir("/data/tmp");
    if (!dir)
        return NULL;
    struct dirent *entry;
    while (!display && (entry = readdir(dir)))
    {
        if (strncmp(entry->d_name, "xauth-", 6))
            continue;
        char path[320];
        snprintf(path, sizeof(path), "/data/tmp/%s", entry->d_name);
        setenv("XAUTHORITY", path, 1);
        display = XOpenDisplay(NULL);
    }
    closedir(dir);
    return display;
}

static int capture(Display *dpy)
{
    Window root = DefaultRootWindow(dpy);
    XWindowAttributes attrs;
    if (!XGetWindowAttributes(dpy, root, &attrs))
        return 1;
    XImage *image = XGetImage(dpy, root, 0, 0, attrs.width, attrs.height, AllPlanes, ZPixmap);
    if (!image)
        return 1;
    printf("P6\n%d %d\n255\n", attrs.width, attrs.height);
    for (int y = 0; y < attrs.height; ++y)
        for (int x = 0; x < attrs.width; ++x)
        {
            unsigned long value = XGetPixel(image, x, y);
            unsigned char rgb[] = {value >> 16, value >> 8, value};
            if (fwrite(rgb, 1, sizeof(rgb), stdout) != sizeof(rgb))
            {
                XDestroyImage(image);
                return 1;
            }
        }
    XDestroyImage(image);
    return 0;
}

static void pause_display(Display *display, unsigned ms)
{
    XFlush(display);
    if (XPending(display))
        return;
    struct pollfd fd = {.fd = ConnectionNumber(display), .events = POLLIN};
    if (poll(&fd, 1, (int)ms) < 0 && errno != EINTR)
        exit(1);
}

static int fake_input(Display *dpy, XExtCodes *codes, unsigned type, unsigned detail, int x, int y)
{
    LockDisplay(dpy);
    xXTestFakeInputReq *request =
        (void *)_XGetRequest(dpy, codes->major_opcode, sz_xXTestFakeInputReq);
    if (request)
    {
        memset((unsigned char *)request + 4, 0, sizeof(*request) - 4);
        request->xtReqType = X_XTestFakeInput;
        request->type = type;
        request->detail = detail;
        if (type == MotionNotify)
        {
            request->root = DefaultRootWindow(dpy);
            request->rootX = x;
            request->rootY = y;
        }
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return request != NULL;
}

static Window create_window(Display *dpy, unsigned width, unsigned height)
{
    Window window = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 32, 96, width, height, 0,
                                        BlackPixel(dpy, DefaultScreen(dpy)),
                                        WhitePixel(dpy, DefaultScreen(dpy)));
    XStoreName(dpy, window, "Desktop response probe");
    XSelectInput(dpy, window,
                 ExposureMask | StructureNotifyMask | ButtonPressMask | ButtonReleaseMask |
                     KeyPressMask | PointerMotionMask);
    XMapRaised(dpy, window);
    XSync(dpy, False);
    uint64_t limit = stamp() + UINT64_C(2000000000);
    XWindowAttributes attributes;
    while (stamp() < limit)
    {
        if (XGetWindowAttributes(dpy, window, &attributes) && attributes.map_state == IsViewable)
            return window;
        poll(NULL, 0, 10);
    }
    XDestroyWindow(dpy, window);
    return None;
}

static uint64_t image_hash(Display *dpy, Window window, unsigned width, unsigned height)
{
    XImage *image =
        XGetImage(dpy, window, (int)width - 32, (int)height - 32, 16, 16, AllPlanes, ZPixmap);
    if (!image)
        return 0;
    uint64_t hash = UINT64_C(1469598103934665603);
    for (unsigned y = 0; y < 16; ++y)
        for (unsigned x = 0; x < 16; ++x)
            hash = (hash ^ XGetPixel(image, x, y)) * UINT64_C(1099511628211);
    XDestroyImage(image);
    return hash;
}

static int input_gate(Display *dpy)
{
    XExtCodes *codes = XInitExtension(dpy, "XTEST");
    if (!codes)
        return 3;
    Window previous_focus, root_return, child_return;
    int previous_revert, root_x, root_y, win_x, win_y;
    unsigned previous_mask;
    XGetInputFocus(dpy, &previous_focus, &previous_revert);
    XQueryPointer(dpy, DefaultRootWindow(dpy), &root_return, &child_return, &root_x, &root_y,
                  &win_x, &win_y, &previous_mask);
    Window window = create_window(dpy, 320, 200);
    if (!window)
        return 1;
    XSetInputFocus(dpy, window, RevertToParent, CurrentTime);
    XWarpPointer(dpy, None, window, 0, 0, 0, 0, 160, 100);
    XSync(dpy, False);
    while (XPending(dpy))
    {
        XEvent event;
        XNextEvent(dpy, &event);
    }
    uint64_t before = image_hash(dpy, window, 320, 200);
    if (!fake_input(dpy, codes, ButtonPress, 5, 0, 0) ||
        !fake_input(dpy, codes, ButtonRelease, 5, 0, 0))
        return 1;
    XFlush(dpy);
    unsigned buttons = 0;
    uint64_t start = stamp(), limit = start + UINT64_C(2000000000);
    while (!buttons && stamp() < limit)
    {
        pause_display(dpy, 20);
        while (XPending(dpy))
        {
            XEvent event;
            XNextEvent(dpy, &event);
            if (event.type == ButtonPress && event.xbutton.window == window &&
                event.xbutton.button == 5)
                ++buttons;
        }
    }
    GC gc = XCreateGC(dpy, window, 0, NULL);
    if (buttons)
    {
        XSetForeground(dpy, gc, BlackPixel(dpy, DefaultScreen(dpy)));
        XFillRectangle(dpy, window, gc, 0, 0, 320, 200);
    }
    XSync(dpy, False);
    uint64_t after = image_hash(dpy, window, 320, 200);
    bool valid = buttons && before && after && before != after;
    printf("{\"type\":\"gate\",\"case\":\"x11-input\",\"status\":\"%s\","
           "\"received_buttons\":%u,\"elapsed_ns\":%llu,\"pixels_changed\":%s,"
           "\"boundary\":\"xserver_readback\"}\n",
           valid ? "valid" : "invalid", buttons, (unsigned long long)(stamp() - start),
           before != after ? "true" : "false");
    XFreeGC(dpy, gc);
    XDestroyWindow(dpy, window);
    XSetInputFocus(dpy, previous_focus, previous_revert, CurrentTime);
    XWarpPointer(dpy, None, DefaultRootWindow(dpy), 0, 0, 0, 0, root_x, root_y);
    XSync(dpy, False);
    return valid ? 0 : 1;
}

static int window_work(Display *dpy, const char *name, unsigned duration)
{
    Window window = create_window(dpy, 1024, 640);
    if (!window)
        return 1;
    GC gc = XCreateGC(dpy, window, 0, NULL);
    unsigned capacity = duration / 20 + 1;
    struct sample *samples = calloc(capacity, sizeof(*samples));
    if (!samples)
        return 1;
    uint64_t first = image_hash(dpy, window, 1024, 640), start = stamp(), next = start;
    unsigned n = 0, moves = 0;
    bool move = !strcmp(name, "window-move");
    int old_x = -1, old_y = -1;
    while (stamp() - start < (uint64_t)duration * UINT64_C(1000000))
    {
        uint64_t now = stamp();
        if (now >= next && n < capacity)
        {
            samples[n].start = now;
            XSetForeground(dpy, gc,
                           n & 1 ? WhitePixel(dpy, DefaultScreen(dpy))
                                 : BlackPixel(dpy, DefaultScreen(dpy)));
            XFillRectangle(dpy, window, gc, 0, 0, 1024, 640);
            if (move)
                XMoveWindow(dpy, window, 32 + (int)(n % 20) * 4, 96 + (int)(n % 10) * 4);
            XSync(dpy, False);
            samples[n].end = stamp();
            ++n;
            next = start + (uint64_t)n * UINT64_C(20000000);
            if (next < samples[n - 1].end)
                next = samples[n - 1].end + UINT64_C(20000000);
        }
        while (XPending(dpy))
        {
            XEvent event;
            XNextEvent(dpy, &event);
            if (event.type == ConfigureNotify &&
                (event.xconfigure.x != old_x || event.xconfigure.y != old_y))
            {
                ++moves;
                old_x = event.xconfigure.x;
                old_y = event.xconfigure.y;
            }
        }
        pause_display(dpy, 1);
    }
    XSetForeground(dpy, gc, BlackPixel(dpy, DefaultScreen(dpy)));
    XFillRectangle(dpy, window, gc, 0, 0, 1024, 640);
    XSync(dpy, False);
    uint64_t last = image_hash(dpy, window, 1024, 640);
    bool valid = n && first && last && first != last && (!move || moves > 1);
    for (unsigned i = 0; i < n; ++i)
        printf("{\"type\":\"sample\",\"case\":\"%s\",\"index\":%u,"
               "\"start_ns\":%llu,\"server_ack_ns\":%llu,\"roundtrip_ns\":%llu}\n",
               name, i, (unsigned long long)samples[i].start, (unsigned long long)samples[i].end,
               (unsigned long long)(samples[i].end - samples[i].start));
    printf("{\"type\":\"summary\",\"case\":\"%s\",\"status\":\"%s\",\"samples\":%u,"
           "\"configure_positions\":%u,\"boundary\":\"xserver_ack_not_scanout\","
           "\"input_route\":\"%s\",\"pixels_changed\":%s}\n",
           name, valid ? "valid" : "invalid", n, moves, move ? "configure_request" : "none",
           first != last ? "true" : "false");
    XFreeGC(dpy, gc);
    XDestroyWindow(dpy, window);
    XSync(dpy, False);
    free(samples);
    return valid ? 0 : 1;
}

static char *window_name(Display *dpy, Window window)
{
    Atom type;
    int format;
    unsigned long count, remaining;
    unsigned char *text = NULL;
    if (XGetWindowProperty(dpy, window, XInternAtom(dpy, "_NET_WM_NAME", False), 0, 512, False,
                           AnyPropertyType, &type, &format, &count, &remaining, &text) != Success ||
        format != 8)
    {
        XFree(text);
        return NULL;
    }
    return (char *)text;
}

static Window find_window(Display *dpy, const char *prefix)
{
    Atom type;
    int format;
    unsigned long count, remaining;
    unsigned char *data = NULL;
    Window found = None;
    XGetWindowProperty(dpy, DefaultRootWindow(dpy), XInternAtom(dpy, "_NET_CLIENT_LIST", False), 0,
                       512, False, AnyPropertyType, &type, &format, &count, &remaining, &data);
    if (data && format == 32)
        for (unsigned long i = 0; i < count; ++i)
        {
            Window window = ((Window *)data)[i];
            char *name = window_name(dpy, window);
            if (name && !strncmp(name, prefix, strlen(prefix)))
                found = window;
            XFree(name);
        }
    XFree(data);
    return found;
}

static int browser_work(Display *dpy, const char *name, const char *prefix, unsigned duration)
{
    uint64_t lookup = stamp(), limit = lookup + UINT64_C(5000000000);
    Window window = None;
    while (!window && stamp() < limit)
    {
        window = find_window(dpy, prefix);
        if (!window)
            poll(NULL, 0, 10);
    }
    if (!window)
    {
        printf("{\"type\":\"gate\",\"case\":\"%s\",\"status\":\"blocked\","
               "\"reason\":\"fixture_window_missing\"}\n",
               name);
        return 1;
    }
    XExtCodes *codes = XInitExtension(dpy, "XTEST");
    XWindowAttributes attrs;
    if (!codes || !XGetWindowAttributes(dpy, window, &attrs))
        return 3;
    XRaiseWindow(dpy, window);
    XSetInputFocus(dpy, window, RevertToParent, CurrentTime);
    XWarpPointer(dpy, None, window, 0, 0, 0, 0, attrs.width / 2, attrs.height / 2);
    int root_x, root_y;
    Window child;
    XTranslateCoordinates(dpy, window, DefaultRootWindow(dpy), attrs.width / 2, attrs.height / 2,
                          &root_x, &root_y, &child);
    fake_input(dpy, codes, MotionNotify, 0, root_x, root_y);
    fake_input(dpy, codes, ButtonPress, 1, 0, 0);
    fake_input(dpy, codes, ButtonRelease, 1, 0, 0);
    if (!strcmp(name, "reload"))
    {
        unsigned control = XKeysymToKeycode(dpy, XK_Control_L);
        unsigned letter = XKeysymToKeycode(dpy, XK_r);
        fake_input(dpy, codes, KeyPress, control, 0, 0);
        fake_input(dpy, codes, KeyPress, letter, 0, 0);
        fake_input(dpy, codes, KeyRelease, letter, 0, 0);
        fake_input(dpy, codes, KeyRelease, control, 0, 0);
    }
    XSync(dpy, False);
    unsigned capacity = duration / 20 + 1;
    struct sample *samples = calloc(capacity, sizeof(*samples));
    if (!samples)
        return 1;
    uint64_t start = stamp(), next = start, gate = start + UINT64_C(3000000000);
    unsigned n = 0, sent = 0;
    long initial = -1, final = -1;
    bool key = !strcmp(name, "scroll-key");
    bool scroll = !strcmp(name, "scroll") || key;
    bool timer = !strcmp(name, "timer-gate");
    uint64_t first_serial = 0, last_serial = 0;
    long wheels = 0, clicks = 0, keys = 0;
    while (stamp() - start < (uint64_t)duration * UINT64_C(1000000))
    {
        uint64_t now = stamp();
        if (scroll && now >= next)
        {
            unsigned detail = key ? XKeysymToKeycode(dpy, XK_space) : 5;
            fake_input(dpy, codes, key ? KeyPress : ButtonPress, detail, 0, 0);
            fake_input(dpy, codes, key ? KeyRelease : ButtonRelease, detail, 0, 0);
            XFlush(dpy);
            ++sent;
            next = now + UINT64_C(33000000);
        }
        char *title = window_name(dpy, window);
        if (title && !strncmp(title, prefix, strlen(prefix)))
        {
            char *end;
            long serial = strtol(title + strlen(prefix), &end, 10);
            if (*end == ':')
            {
                final = strtol(end + 1, &end, 10);
                if (*end == ':')
                {
                    wheels = strtol(end + 1, &end, 10);
                    if (*end == ':')
                        clicks = strtol(end + 1, &end, 10);
                    if (*end == ':')
                        keys = strtol(end + 1, &end, 10);
                }
                if (initial < 0)
                {
                    initial = final;
                    first_serial = (uint64_t)serial;
                }
                last_serial = (uint64_t)serial;
                samples[n++] = (struct sample){now, (uint64_t)serial, final};
            }
        }
        XFree(title);
        if (n >= capacity || (scroll && now >= gate && final == initial))
            break;
        poll(NULL, 0, 20);
    }
    bool valid = n && initial >= 0 && (!scroll || final - initial > 1000) &&
                 (!timer || last_serial >= first_serial + 2);
    printf("{\"type\":\"summary\",\"case\":\"%s\",\"status\":\"%s\","
           "\"samples\":%u,\"input_events\":%u,\"start_y\":%ld,\"end_y\":%ld,"
           "\"first_serial\":%llu,\"last_serial\":%llu,"
           "\"wheel_callbacks\":%ld,\"pointer_down_callbacks\":%ld,\"key_callbacks\":%ld,"
           "\"boundary\":\"dom_progress_not_scanout\",\"frame_latency_ns\":null}\n",
           name, valid ? "valid" : "invalid", n, sent, initial, final,
           (unsigned long long)first_serial, (unsigned long long)last_serial, wheels, clicks, keys);
    for (unsigned i = 0; i < n; ++i)
        printf("{\"type\":\"sample\",\"case\":\"%s\",\"observed_ns\":%llu,"
               "\"serial\":%llu,\"scroll_y\":%ld}\n",
               name, (unsigned long long)samples[i].start, (unsigned long long)samples[i].end,
               samples[i].position);
    free(samples);
    return valid ? 0 : 1;
}

static int run(int argc, char **argv)
{
    const char *name = "input-gate", *prefix = "LBENCH:";
    unsigned duration = 1000;
    bool scan_auth = false;
    for (int i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "--case") && i + 1 < argc)
            name = argv[++i];
        else if (!strcmp(argv[i], "--title-prefix") && i + 1 < argc)
            prefix = argv[++i];
        else if (!strcmp(argv[i], "--duration-ms") && i + 1 < argc)
        {
            char *end;
            unsigned long value = strtoul(argv[++i], &end, 10);
            if (*end || value < 100 || value > 60000)
                return 2;
            duration = (unsigned)value;
        }
        else if (!strcmp(argv[i], "--scan-auth"))
            scan_auth = true;
        else
            return 2;
    }
    Display *display = open_display(scan_auth);
    if (!display)
    {
        puts("{\"type\":\"gate\",\"status\":\"blocked\",\"reason\":\"display_unavailable\"}");
        return 1;
    }
    int result = !strcmp(name, "capture")      ? capture(display)
                 : !strcmp(name, "input-gate") ? input_gate(display)
                 : !strcmp(name, "window-move") || !strcmp(name, "roundtrip")
                     ? window_work(display, name, duration)
                 : !strcmp(name, "scroll") || !strcmp(name, "scroll-key") ||
                         !strcmp(name, "observe") || !strcmp(name, "timer-gate") ||
                         !strcmp(name, "reload")
                     ? browser_work(display, name, prefix, duration)
                     : 2;
    XCloseDisplay(display);
    return result;
}

int main(int argc, char **argv)
{
    unsigned budget = 9000;
    for (int i = 1; i + 1 < argc; ++i)
        if (!strcmp(argv[i], "--duration-ms"))
        {
            char *end;
            unsigned long value = strtoul(argv[i + 1], &end, 10);
            if (*end || value < 100 || value > 60000)
                return 2;
            budget += (unsigned)value;
        }
    pid_t child = fork();
    if (child < 0)
        return 1;
    if (!child)
    {
        if (setsid() < 0)
            _exit(1);
        int result = run(argc, argv);
        fflush(stdout);
        _exit(result);
    }
    signal(SIGINT, stop);
    signal(SIGTERM, stop);
    signal(SIGHUP, stop);
    uint64_t limit = stamp() + (uint64_t)budget * UINT64_C(1000000);
    int status;
    for (;;)
    {
        pid_t joined = waitpid(child, &status, WNOHANG);
        if (joined == child)
            return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
        if (joined < 0 && errno != EINTR)
            return 1;
        if (interrupted || stamp() >= limit)
        {
            kill(-child, SIGKILL);
            kill(child, SIGKILL);
            while (waitpid(child, &status, 0) < 0 && errno == EINTR)
            {
            }
            puts("{\"type\":\"supervisor\",\"status\":\"invalid\",\"reason\":\"deadline\"}");
            return 1;
        }
        poll(NULL, 0, 20);
    }
}
