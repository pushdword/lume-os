/* SPDX-License-Identifier: BSD-3-Clause */
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/Xlibint.h>
#include <X11/Xos.h>
#include <X11/extensions/xtestproto.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <locale.h>
#include <wchar.h>
#include "lume/process.h"
#include "lume/abi.h"
#include "lume/syscall_native.h"

static FILE *console;

static void report(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
    if (console)
    {
        va_start(ap, format);
        vfprintf(console, format, ap);
        va_end(ap);
        fflush(console);
    }
}

static int locale_environment_test(void)
{
    unsetenv("LC_ALL");
    unsetenv("LC_CTYPE");
    setenv("LANG", "C.UTF-8", 1);
    const char *selected = setlocale(LC_ALL, "");
    int failed = !selected || strcmp(selected, "C.UTF-8");
    setenv("LC_CTYPE", "C", 1);
    selected = setlocale(LC_ALL, "");
    failed |= !selected || strcmp(selected, "C");
    setenv("LC_ALL", "C.UTF-8", 1);
    selected = setlocale(LC_CTYPE, "");
    failed |= !selected || strcmp(selected, "C.UTF-8");
    setenv("LC_ALL", "", 1);
    selected = setlocale(LC_CTYPE, "");
    failed |= !selected || strcmp(selected, "C");
    unsetenv("LC_ALL");
    unsetenv("LC_CTYPE");
    unsetenv("LANG");
    selected = setlocale(LC_ALL, "");
    failed |= !selected || strcmp(selected, "C");
    setenv("LANG", "C.UTF-8", 1);
    selected = setlocale(LC_ALL, "C");
    failed |= !selected || strcmp(selected, "C");
    report("X11_LOCALE_ENV result=%d\n", !failed);
    return failed;
}

static int blocking_socket_test(void)
{
    int listener = socket(AF_UNIX, SOCK_STREAM, 0), barrier[2];
    struct sockaddr_un address = {.sun_family = AF_UNIX};
    strcpy(address.sun_path, "/data/tmp/socket-block");
    if (listener < 0 || bind(listener, (struct sockaddr *)&address, sizeof(address)) ||
        listen(listener, 1) || pipe(barrier))
        return 1;
    pid_t child = fork();
    if (child < 0)
        return 1;
    if (!child)
    {
        close(listener);
        close(barrier[0]);
        int first = socket(AF_UNIX, SOCK_STREAM, 0);
        int second = socket(AF_UNIX, SOCK_STREAM, 0);
        if (first < 0 || second < 0 ||
            connect(first, (struct sockaddr *)&address, sizeof(address)) ||
            write(barrier[1], "1", 1) != 1 ||
            connect(second, (struct sockaddr *)&address, sizeof(address)) ||
            write(barrier[1], "2", 1) != 1)
            _exit(1);
        close(first);
        close(second);
        _exit(0);
    }
    close(barrier[1]);
    char byte;
    if (read(barrier[0], &byte, 1) != 1 || byte != '1')
        return 1;
    int failed = 0;
    struct pollfd pending = {.fd = barrier[0], .events = POLLIN};
    failed |= poll(&pending, 1, 100) != 0;
    int first = accept(listener, NULL, NULL);
    failed |= first < 0;
    failed |= read(barrier[0], &byte, 1) != 1 || byte != '2';
    int second = accept(listener, NULL, NULL), status = 0;
    failed |= second < 0 || waitpid(child, &status, 0) != child || !WIFEXITED(status) ||
              WEXITSTATUS(status);
    close(first);
    close(second);
    close(listener);
    close(barrier[0]);
    report("X11_SOCKET_BLOCKING result=%d\n", !failed);
    return failed;
}

static int fake_input(Display *display, unsigned type, unsigned detail, int x, int y)
{
    int opcode, event, error;
    if (!XQueryExtension(display, "XTEST", &opcode, &event, &error))
        return 1;
    xXTestFakeInputReq *request = _XGetRequest(display, opcode, sizeof(*request));
    memset((char *)request + sizeof(xReq), 0, sizeof(*request) - sizeof(xReq));
    request->xtReqType = X_XTestFakeInput;
    request->type = type;
    request->detail = detail;
    request->root = DefaultRootWindow(display);
    request->rootX = x;
    request->rootY = y;
    XSync(display, False);
    poll(NULL, 0, 20);
    return 0;
}

static int fake_chord(Display *display, const char *chord)
{
    char *copy = strdup(chord);
    if (!copy)
        return 1;
    char *saved = NULL;
    unsigned keys[8], count = 0;
    for (char *name = strtok_r(copy, "+", &saved); name && count < 8;
         name = strtok_r(NULL, "+", &saved))
    {
        unsigned code = XKeysymToKeycode(display, XStringToKeysym(name));
        if (!code)
        {
            free(copy);
            return 1;
        }
        keys[count++] = code;
    }
    free(copy);
    report("X11_CHORD keys=%u\n", count);
    for (unsigned i = 0; i < count; ++i)
        if (fake_input(display, KeyPress, keys[i], 0, 0))
            return 1;
    while (count)
        if (fake_input(display, KeyRelease, keys[--count], 0, 0))
            return 1;
    return 0;
}

static int fake_symbol(Display *display, KeySym symbol)
{
    for (unsigned code = 8; code < 256; ++code)
        for (unsigned level = 0; level < 4; ++level)
            if (XkbKeycodeToKeysym(display, code, 0, level) == symbol)
            {
                unsigned shift = XKeysymToKeycode(display, XStringToKeysym("Shift_L"));
                unsigned third = XKeysymToKeycode(display, XStringToKeysym("ISO_Level3_Shift"));
                if (level & 2)
                    fake_input(display, KeyPress, third, 0, 0);
                if (level & 1)
                    fake_input(display, KeyPress, shift, 0, 0);
                fake_input(display, KeyPress, code, 0, 0);
                fake_input(display, KeyRelease, code, 0, 0);
                if (level & 1)
                    fake_input(display, KeyRelease, shift, 0, 0);
                if (level & 2)
                    fake_input(display, KeyRelease, third, 0, 0);
                return 0;
            }
    return 1;
}

static int control(Display *display, int argc, char **argv)
{
    if (argc < 2)
        return -1;
    if (argc == 2 && !strcmp(argv[1], "keymap"))
    {
        const char *names[] = {"Control_L",  "Alt_L",           "t", "dead_acute", "a",
                               "dead_tilde", "dead_circumflex", "o", "ccedilla"};
        for (unsigned i = 0; i < sizeof(names) / sizeof(*names); ++i)
        {
            unsigned code = XKeysymToKeycode(display, XStringToKeysym(names[i]));
            report("X11_KEYMAP name=%s code=%u levels=%lx,%lx,%lx,%lx\n", names[i], code,
                   XkbKeycodeToKeysym(display, code, 0, 0), XkbKeycodeToKeysym(display, code, 0, 1),
                   XkbKeycodeToKeysym(display, code, 0, 2),
                   XkbKeycodeToKeysym(display, code, 0, 3));
        }
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "compose"))
    {
        setlocale(LC_ALL, "");
        XSetLocaleModifiers("@im=none");
        XIM method = XOpenIM(display, NULL, NULL, NULL);
        Window window =
            XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, 100, 100, 0, 0, 0);
        XIC context = method ? XCreateIC(method, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
                                         XNClientWindow, window, XNFocusWindow, window, NULL)
                             : NULL;
        if (!context)
            return 1;
        XSetICFocus(context);
        const char *names[] = {"dead_acute", "a"};
        int composed = 0;
        for (unsigned i = 0; i < 2; ++i)
        {
            XEvent event = {0};
            event.xkey.type = KeyPress;
            event.xkey.display = display;
            event.xkey.window = window;
            event.xkey.root = DefaultRootWindow(display);
            event.xkey.same_screen = True;
            event.xkey.keycode = XKeysymToKeycode(display, XStringToKeysym(names[i]));
            int filtered = XFilterEvent(&event, window);
            report("X11_COMPOSE key=%s filtered=%d\n", names[i], filtered);
        }
        XEvent event;
        while (XCheckTypedWindowEvent(display, window, KeyPress, &event))
        {
            char bytes[32];
            KeySym symbol;
            Status status;
            int count =
                Xutf8LookupString(context, &event.xkey, bytes, sizeof(bytes), &symbol, &status);
            report("X11_COMPOSE status=%d bytes=%d symbol=%lx\n", status, count, symbol);
            composed |=
                count == 2 && (unsigned char)bytes[0] == 0xc3 && (unsigned char)bytes[1] == 0xa1;
        }
        XDestroyIC(context);
        XCloseIM(method);
        XDestroyWindow(display, window);
        report("X11_COMPOSE result=%d\n", composed);
        return !composed;
    }
    if (!strcmp(argv[1], "key") || !strcmp(argv[1], "text") || !strcmp(argv[1], "drag"))
    {
        int opcode, event, error;
        if (!XQueryExtension(display, "XTEST", &opcode, &event, &error))
            return 1;
        xXTestGrabControlReq *grab = _XGetRequest(display, opcode, sizeof(*grab));
        memset((char *)grab + sizeof(xReq), 0, sizeof(*grab) - sizeof(xReq));
        grab->xtReqType = X_XTestGrabControl;
        grab->impervious = True;
        XSync(display, False);
        if (argc == 3 && !strcmp(argv[1], "key"))
            return fake_chord(display, argv[2]);
        if (argc == 3 && !strcmp(argv[1], "text"))
        {
            for (const unsigned char *p = (const unsigned char *)argv[2]; *p; ++p)
                if (*p > 127 || fake_symbol(display, *p))
                    return 1;
            return 0;
        }
        if (argc == 6 && !strcmp(argv[1], "drag"))
        {
            int x = atoi(argv[2]), y = atoi(argv[3]), dx = atoi(argv[4]), dy = atoi(argv[5]);
            fake_input(display, MotionNotify, 0, x, y);
            fake_input(display, ButtonPress, 1, 0, 0);
            for (int step = 1; step <= 12; ++step)
                fake_input(display, MotionNotify, 0, x + dx * step / 12, y + dy * step / 12);
            return fake_input(display, ButtonRelease, 1, 0, 0);
        }
        return 1;
    }
    if (!strcmp(argv[1], "terminals") || !strcmp(argv[1], "programs"))
    {
        int programs = !strcmp(argv[1], "programs");
        for (unsigned i = 0; i < 2; ++i)
        {
            pid_t child = fork();
            if (!child)
            {
                close(ConnectionNumber(display));
                const char *title =
                    programs ? (i ? "Vim UTF-8" : "htop") : (i ? "Terminal 3" : "Terminal 2");
                const char *geometry = i ? "48x16+340+220" : "48x16+220+140";
                if (programs && i)
                    execl("/usr/bin/xterm", "xterm", "-title", title, "-fn", "9x15", "-geometry",
                          geometry, "-e", "/usr/bin/vi", "-u", "NONE", "-i", "NONE", "-n",
                          "/data/tmp/terminal-utf8", NULL);
                else
                    execl("/usr/bin/xterm", "xterm", "-title", title, "-fn", "9x15", "-geometry",
                          geometry, "-e", programs ? "/usr/bin/htop" : "/bin/sh", NULL);
                _exit(127);
            }
            if (child < 0)
                return 1;
            report("X11_LAUNCH pid=%ld\n", (long)child);
        }
        return 0;
    }
    if (argc < 3)
        return -1;
    Window window = strtoul(argv[2], NULL, 0);
    XEvent event = {0};
    event.xclient.type = ClientMessage;
    event.xclient.display = display;
    event.xclient.window = window;
    event.xclient.format = 32;
    if (!strcmp(argv[1], "geometry") && argc == 7)
    {
        event.xclient.message_type = XInternAtom(display, "_NET_MOVERESIZE_WINDOW", False);
        event.xclient.data.l[0] = NorthWestGravity | (15 << 8) | (2 << 12);
        for (unsigned i = 0; i < 4; ++i)
            event.xclient.data.l[i + 1] = strtol(argv[i + 3], NULL, 10);
    }
    else if (!strcmp(argv[1], "maximize") || !strcmp(argv[1], "restore"))
    {
        event.xclient.message_type = XInternAtom(display, "_NET_WM_STATE", False);
        event.xclient.data.l[0] = !strcmp(argv[1], "maximize");
        event.xclient.data.l[1] = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
        event.xclient.data.l[2] = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
        event.xclient.data.l[3] = 2;
    }
    else if (!strcmp(argv[1], "focus"))
    {
        XMapRaised(display, window);
        event.xclient.message_type = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
        event.xclient.data.l[0] = 2;
    }
    else if (!strcmp(argv[1], "minimize"))
    {
        return !XIconifyWindow(display, window, DefaultScreen(display));
    }
    else if (!strcmp(argv[1], "close"))
    {
        event.xclient.message_type = XInternAtom(display, "_NET_CLOSE_WINDOW", False);
        event.xclient.data.l[1] = 2;
    }
    else
        return -1;
    int sent = XSendEvent(display, DefaultRootWindow(display), False,
                          SubstructureRedirectMask | SubstructureNotifyMask, &event);
    XSync(display, False);
    report("X11_CONTROL action=%s window=%lu sent=%d\n", argv[1], window, sent != 0);
    return !sent;
}

int main(int argc, char **argv)
{
    int fd = open("/dev/console", O_WRONLY);
    if (fd >= 0)
        console = fdopen(fd, "w");
    struct lume_memory_snapshot memory;
    if (!lume_native_call(LUME_SYS_MEMORY, (uintptr_t)&memory, sizeof(memory), 0))
        report("X11_MEMORY total=%llu free=%llu process=%llu\n",
               (unsigned long long)memory.total_pages, (unsigned long long)memory.free_pages,
               (unsigned long long)memory.process_pages);
    pid_t server = -1;
    for (unsigned i = 0; i < 128; ++i)
    {
        struct lume_task_info task;
        if (lume_native_call(LUME_SYS_PROCESS, i, (uintptr_t)&task, sizeof(task)) <= 0)
            continue;
        report("X11_TASK pid=%u parent=%u state=%u name=%s\n", task.pid, task.parent, task.state,
               task.name);
        if (!strcmp(task.name, "Xlume"))
            server = task.pid;
    }
    struct winsize size;
    if (!ioctl(0, TIOCGWINSZ, &size))
        report("X11_TTY name=%s rows=%u cols=%u\n", ttyname(0), size.ws_row, size.ws_col);
    if (argc >= 2 && !strcmp(argv[1], "crash"))
    {
        if (argc == 3)
            server = atoi(argv[2]);
        report("X11_SERVER_KILL pid=%ld\n", (long)server);
        return server > 0 ? kill(server, SIGKILL) != 0 : 1;
    }
    if (argc == 2 && !strcmp(argv[1], "timers"))
    {
        struct timeval before, after;
        struct timespec monotonic;
        if (X_GETTIMEOFDAY(&before) || clock_gettime(CLOCK_MONOTONIC, &monotonic) ||
            before.tv_sec != monotonic.tv_sec)
            return 1;
        usleep(30000);
        if (X_GETTIMEOFDAY(&after))
            return 1;
        int64_t elapsed =
            (int64_t)(after.tv_sec - before.tv_sec) * 1000000 + after.tv_usec - before.tv_usec;
        if (elapsed < 20000 || elapsed > 2000000)
            return 1;
        report("X11_TIMERS_PASS elapsed_us=%lld\n", (long long)elapsed);
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "system"))
        return 0;
    if (argc == 2 && !strcmp(argv[1], "locale"))
    {
        if (locale_environment_test())
            return 1;
        char before[64];
        snprintf(before, sizeof(before), "%s", setlocale(LC_CTYPE, NULL));
        const char *selected = setlocale(LC_ALL, "");
        char bytes[16];
        mbstate_t state = {0};
        size_t count = wcrtomb(bytes, L'\u00e7', &state);
        int supported = XSupportsLocale();
        const char *modifiers = XSetLocaleModifiers("@im=none");
        Display *display = XOpenDisplay(NULL);
        XIM method = display ? XOpenIM(display, NULL, NULL, NULL) : NULL;
        report("X11_LOCALE before=%s selected=%s max=%u bytes=%u supported=%d modifiers=%d im=%d\n",
               before, selected ? selected : "unavailable", (unsigned)MB_CUR_MAX, (unsigned)count,
               supported, modifiers != NULL, method != NULL);
        if (method)
            XCloseIM(method);
        if (display)
            XCloseDisplay(display);
        return !selected || count != 2 || (unsigned char)bytes[0] != 0xc3 ||
               (unsigned char)bytes[1] != 0xa7 || !supported || !modifiers || !method;
    }
    if (argc == 2 && !strcmp(argv[1], "sockets"))
        return blocking_socket_test();
    if (argc == 2 && !strcmp(argv[1], "files"))
    {
        const char *paths[] = {"/data/tmp/pt.txt", "/data/tmp/vi.txt", "/data/tmp/count.txt"};
        for (unsigned i = 0; i < sizeof(paths) / sizeof(*paths); ++i)
        {
            FILE *file = fopen(paths[i], "r");
            unsigned char bytes[256];
            size_t count = file ? fread(bytes, 1, sizeof(bytes), file) : 0;
            report("X11_FILE path=%s bytes=%u hex=", paths[i], (unsigned)count);
            for (size_t n = 0; n < count; ++n)
                report("%02x", bytes[n]);
            report("\n");
            if (file)
                fclose(file);
        }
        return 0;
    }
    int unauthenticated = argc == 2 && !strcmp(argv[1], "unauth");
    if (unauthenticated)
        setenv("XAUTHORITY", "/data/tmp/absent-xauthority", 1);
    Display *display = XOpenDisplay(NULL);
    if (unauthenticated)
    {
        report("X11_AUTH_REJECTED result=%d\n", display == NULL);
        if (display)
            XCloseDisplay(display);
        return display != NULL;
    }
    if (!display)
    {
        report("X11_CHECK_FAIL display\n");
        return 1;
    }
    int controlled = control(display, argc, argv);
    if (controlled >= 0)
    {
        XCloseDisplay(display);
        return controlled;
    }
    Window root = DefaultRootWindow(display);
    Window pointer_root, pointer_child;
    int root_x, root_y, child_x, child_y;
    unsigned pointer_mask;
    XQueryPointer(display, root, &pointer_root, &pointer_child, &root_x, &root_y, &child_x,
                  &child_y, &pointer_mask);
    report("X11_POINTER x=%d y=%d mask=%u\n", root_x, root_y, pointer_mask);
    report("X11_DISPLAY width=%d height=%d depth=%d\n", DisplayWidth(display, 0),
           DisplayHeight(display, 0), DefaultDepth(display, 0));
    const char *extensions[] = {"RENDER", "SHAPE", "Composite", "DAMAGE", "XFIXES", "MIT-SHM"};
    for (unsigned i = 0; i < sizeof(extensions) / sizeof(*extensions); ++i)
    {
        int opcode, event, error;
        report("X11_EXTENSION name=%s present=%d\n", extensions[i],
               XQueryExtension(display, extensions[i], &opcode, &event, &error));
    }
    Atom type;
    int format;
    unsigned long count = 0, after;
    unsigned char *data = NULL;
    XGetWindowProperty(display, root, XInternAtom(display, "_NET_CLIENT_LIST", False), 0, 128,
                       False, XA_WINDOW, &type, &format, &count, &after, &data);
    report("X11_WINDOWS count=%lu\n", count);
    Window focus;
    int revert;
    XGetInputFocus(display, &focus, &revert);
    for (unsigned long i = 0; data && i < count; ++i)
    {
        Window window = ((Window *)data)[i], child;
        XWindowAttributes attributes;
        int x, y;
        char *name = NULL;
        XGetWindowAttributes(display, window, &attributes);
        XTranslateCoordinates(display, window, root, 0, 0, &x, &y, &child);
        XFetchName(display, window, &name);
        report("X11_WINDOW id=%lu name=%s x=%d y=%d width=%d height=%d mapped=%d focus=%d\n",
               window, name ? name : "", x, y, attributes.width, attributes.height,
               attributes.map_state, window == focus);
        XFree(name);
    }
    XFree(data);
    XCloseDisplay(display);
    report("X11_CHECK_PASS\n");
    if (console)
        fclose(console);
    return 0;
}
