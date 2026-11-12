/* SPDX-License-Identifier: BSD-3-Clause */
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include "lume/abi.h"
#include "lume/activity.h"
#include "lume/process.h"
#include "lume/syscall_native.h"
#include "perf.h"

static uint64_t milliseconds(void)
{
    return (uint64_t)lume_native_call(LUME_SYS_CLOCK, 0, 0, 0);
}

static Display *display_open(void)
{
    Display *display = NULL;
    DIR *directory = opendir("/data/tmp");
    if (!directory)
        return NULL;
    struct dirent *entry;
    while (!display && (entry = readdir(directory)))
    {
        if (strncmp(entry->d_name, "xauth-", 6))
            continue;
        char path[320];
        snprintf(path, sizeof(path), "/data/tmp/%s", entry->d_name);
        setenv("XAUTHORITY", path, 1);
        display = XOpenDisplay("unix/:0");
    }
    closedir(directory);
    return display;
}

static unsigned long window_pid(Display *display, Window window)
{
    Atom type;
    int format;
    unsigned long count, remaining, pid = 0;
    unsigned char *data = NULL;
    if (XGetWindowProperty(display, window, XInternAtom(display, "_NET_WM_PID", False), 0, 1, False,
                           XA_CARDINAL, &type, &format, &count, &remaining, &data) == Success &&
        type == XA_CARDINAL && format == 32 && count)
        pid = *(unsigned long *)data;
    XFree(data);
    return pid;
}

static Window browser_window(Display *display, Atom clients, pid_t owner)
{
    Atom type;
    int format;
    unsigned long count, remaining;
    unsigned char *data = NULL;
    Window found = None;
    if (XGetWindowProperty(display, DefaultRootWindow(display), clients, 0, 128, False,
                           AnyPropertyType, &type, &format, &count, &remaining, &data) == Success &&
        format == 32)
        for (unsigned long i = 0; i < count; ++i)
        {
            Window window = ((Window *)data)[i];
            XClassHint hint = {0};
            XWindowAttributes attributes;
            XGetClassHint(display, window, &hint);
            unsigned long pid = window_pid(display, window);
            bool matches = owner > 0 && pid == (unsigned long)owner;
            if (!owner && pid)
            {
                struct lume_task_info info;
                matches = lume_native_call(LUME_SYS_PROCESS, pid - 1, (uintptr_t)&info,
                                           sizeof(info)) > 0 &&
                          !strcmp(info.name, "CrBrowserMain");
            }
            if (!owner && hint.res_class)
                matches = matches || strstr(hint.res_class, "Chromium") ||
                          strstr(hint.res_class, "chromium") ||
                          strstr(hint.res_class, "Google-chrome");
            if (matches && XGetWindowAttributes(display, window, &attributes) &&
                attributes.map_state == IsViewable)
                found = window;
            XFree(hint.res_name);
            XFree(hint.res_class);
        }
    XFree(data);
    return found;
}

static void inspect_windows(Display *display, Window window, unsigned depth, unsigned *budget)
{
    if (!*budget || depth > 3)
        return;
    --*budget;
    XClassHint hint = {0};
    XWindowAttributes attributes;
    if (XGetWindowAttributes(display, window, &attributes))
    {
        XGetClassHint(display, window, &hint);
        printf("STARTUP_XWINDOW id=%lu depth=%u mapped=%d size=%dx%d pid=%lu class=%s\n", window,
               depth, attributes.map_state, attributes.width, attributes.height,
               window_pid(display, window), hint.res_class ? hint.res_class : "-");
        XFree(hint.res_name);
        XFree(hint.res_class);
    }
    Window root, parent, *children = NULL;
    unsigned count;
    if (XQueryTree(display, window, &root, &parent, &children, &count))
        for (unsigned i = 0; i < count && *budget; ++i)
            inspect_windows(display, children[i], depth + 1, budget);
    XFree(children);
}

static int desktop_environment(void)
{
    if (getenv("DBUS_SESSION_BUS_ADDRESS"))
        return 0;
    DIR *directory = opendir("/data/tmp");
    if (!directory)
        return -1;
    char prefix[48];
    snprintf(prefix, sizeof(prefix), "xfce-%lu-", (unsigned long)getuid());
    struct dirent *entry;
    int result = -1;
    while ((entry = readdir(directory)))
    {
        if (strncmp(entry->d_name, prefix, strlen(prefix)))
            continue;
        char runtime[320], bus_path[336], address[352];
        struct stat metadata;
        snprintf(runtime, sizeof(runtime), "/data/tmp/%s", entry->d_name);
        snprintf(bus_path, sizeof(bus_path), "%s/bus", runtime);
        if (stat(runtime, &metadata) || !S_ISDIR(metadata.st_mode) || metadata.st_uid != getuid() ||
            (metadata.st_mode & 077))
            continue;
        struct sockaddr_un endpoint = {.sun_family = AF_UNIX};
        if (strlen(bus_path) >= sizeof(endpoint.sun_path))
            continue;
        strcpy(endpoint.sun_path, bus_path);
        int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
        if (fd < 0)
            continue;
        int connected = connect(fd, (struct sockaddr *)&endpoint, sizeof(endpoint));
        int error = errno;
        close(fd);
        if (connected && error != EINPROGRESS)
            continue;
        snprintf(address, sizeof(address), "unix:path=%s", bus_path);
        result = setenv("DBUS_SESSION_BUS_ADDRESS", address, 1) ||
                 setenv("XDG_RUNTIME_DIR", runtime, 1) ||
                 setenv("XDG_CURRENT_DESKTOP", "XFCE", 1) || setenv("DESKTOP_SESSION", "xfce", 1) ||
                 setenv("NO_AT_BRIDGE", "1", 1) || setenv("XDG_CONFIG_DIRS", "/etc/xdg", 1) ||
                 setenv("XDG_DATA_DIRS", "/data/share:/usr/share", 1);
        break;
    }
    closedir(directory);
    return result;
}

static bool browser_running(void)
{
    struct lume_system_features features;
    if (lume_native_call(LUME_SYS_FEATURES, (uintptr_t)&features, sizeof(features),
                         LUME_FEATURE_VERSION))
        return true;
    for (unsigned i = 0; i < features.task_limit; ++i)
    {
        struct lume_task_info info;
        if (lume_native_call(LUME_SYS_PROCESS, i, (uintptr_t)&info, sizeof(info)) > 0 &&
            info.state != 4 &&
            (!strcmp(info.name, "CrBrowserMain") || !strcmp(info.name, "chromium") ||
             !strcmp(info.name, "browser")))
            return true;
    }
    return false;
}

static int activity(unsigned operation, struct lume_activity_snapshot *snapshot)
{
    return (int)lume_native_call(LUME_SYS_TELEMETRY, (uintptr_t)snapshot, sizeof(*snapshot),
                                 operation);
}

static void sample(uint64_t start, struct lume_perf_snapshot *previous, pid_t child)
{
    struct lume_perf_snapshot current;
    if (lume_native_call(LUME_SYS_TELEMETRY, (uintptr_t)&current, sizeof(current),
                         LUME_PERF_VERSION))
        return;
    printf("STARTUP_SAMPLE ms=%llu hz=%u", (unsigned long long)(milliseconds() - start),
           current.clock_hz);
    struct lume_task_info info;
    if (child > 0 &&
        lume_native_call(LUME_SYS_PROCESS, (unsigned)child - 1, (uintptr_t)&info, sizeof(info)) > 0)
        printf(" child_state=%u child_name=%s", info.state, info.name);
    for (unsigned c = 0; c < current.core_count; ++c)
    {
        printf(" c%u=%u,%u,%u,%u,%u", c, current.cores[c].counter - previous->cores[c].counter,
               current.cores[c].ticks[LUME_PERF_USER] - previous->cores[c].ticks[LUME_PERF_USER],
               current.cores[c].ticks[LUME_PERF_KERNEL] -
                   previous->cores[c].ticks[LUME_PERF_KERNEL],
               current.cores[c].ticks[LUME_PERF_LOCKWAIT] -
                   previous->cores[c].ticks[LUME_PERF_LOCKWAIT],
               current.cores[c].ticks[LUME_PERF_IDLE] - previous->cores[c].ticks[LUME_PERF_IDLE]);
    }
    struct lume_activity_snapshot work;
    if (!activity(LUME_ACTIVITY_READ, &work))
    {
        printf(" activity_hz=%llu", (unsigned long long)work.clock_hz);
        for (unsigned k = 0; k < LUME_ACTIVITY_COUNT; ++k)
        {
            uint64_t ticks = 0, calls = 0;
            for (unsigned c = 0; c < work.core_count; ++c)
            {
                ticks += work.rows[c][k].ticks;
                calls += work.rows[c][k].calls;
            }
            printf(" a%u=%llu,%llu", k, (unsigned long long)ticks, (unsigned long long)calls);
        }
    }
    struct lume_memory_snapshot memory;
    if (!lume_native_call(LUME_SYS_MEMORY, (uintptr_t)&memory, sizeof(memory), 0))
        printf(" free_pages=%llu", (unsigned long long)memory.free_pages);
    puts("");
    *previous = current;
}

int main(int argc, char **argv)
{
    if (argc == 3 && !strcmp(argv[1], "--map"))
    {
        setvbuf(stdout, NULL, _IONBF, 0);
        uint64_t start = milliseconds();
        int fd = open(argv[2], O_RDONLY | O_CLOEXEC);
        struct stat info;
        if (fd < 0 || fstat(fd, &info) || info.st_size <= 0 || !S_ISREG(info.st_mode))
            return perror("STARTUP_MAP open"), 1;
        printf("STARTUP_MAP_OPEN ms=%llu bytes=%llu\n",
               (unsigned long long)(milliseconds() - start), (unsigned long long)info.st_size);
        const volatile unsigned char *data =
            mmap(NULL, info.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (data == MAP_FAILED)
            return perror("STARTUP_MAP mmap"), 1;
        printf("STARTUP_MAP_READY ms=%llu\n", (unsigned long long)(milliseconds() - start));
        unsigned checksum = 0;
        for (uint64_t i = 0; i < (uint64_t)info.st_size; i += 4096)
            checksum ^= data[i];
        printf("STARTUP_MAP_DONE ms=%llu checksum=%u\n",
               (unsigned long long)(milliseconds() - start), checksum);
        munmap((void *)data, info.st_size);
        close(fd);
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "--inspect"))
    {
        struct lume_system_features features;
        if (lume_native_call(LUME_SYS_FEATURES, (uintptr_t)&features, sizeof(features),
                             LUME_FEATURE_VERSION))
            return 1;
        for (unsigned i = 0; i < features.task_limit; ++i)
        {
            struct lume_task_info info;
            if (lume_native_call(LUME_SYS_PROCESS, i, (uintptr_t)&info, sizeof(info)) > 0)
                printf("STARTUP_PROCESS pid=%u parent=%u state=%u runs=%u name=%s\n", info.pid,
                       info.parent, info.state, info.runs, info.name);
        }
        Display *display = display_open();
        if (display)
        {
            Atom clients = XInternAtom(display, "_NET_CLIENT_LIST", False);
            printf("STARTUP_VISIBLE id=%lu\n", browser_window(display, clients, 0));
            unsigned budget = 128;
            inspect_windows(display, DefaultRootWindow(display), 0, &budget);
            XCloseDisplay(display);
        }
        return display ? 0 : 1;
    }
    if (argc < 3)
        return 2;
    setvbuf(stdout, NULL, _IONBF, 0);
    setenv("DISPLAY", "unix/:0", 1);
    Display *display = display_open();
    if (!display)
        return fprintf(stderr, "STARTUP_ERROR display\n"), 1;
    Atom clients = XInternAtom(display, "_NET_CLIENT_LIST", False);
    if (desktop_environment())
        return fprintf(stderr, "STARTUP_ERROR desktop_environment\n"), 1;
    if (browser_running() || browser_window(display, clients, 0))
        return fprintf(stderr, "STARTUP_ERROR browser_already_open\n"), 1;
    int log = open(argv[1], O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    if (log < 0)
        return perror("STARTUP_ERROR log"), 1;
    struct lume_activity_snapshot work;
    int activity_owned = !activity(LUME_ACTIVITY_READ, &work) && !work.enabled &&
                         !activity(LUME_ACTIVITY_START, &work);
    struct lume_perf_snapshot previous;
    if (lume_native_call(LUME_SYS_TELEMETRY, (uintptr_t)&previous, sizeof(previous),
                         LUME_PERF_VERSION))
    {
        if (activity_owned)
            activity(LUME_ACTIVITY_STOP, &work);
        close(log);
        XCloseDisplay(display);
        return 1;
    }
    struct timespec utc;
    int clock_ready = !clock_gettime(CLOCK_REALTIME, &utc);
    uint64_t start = milliseconds();
    printf("STARTUP_BEGIN ms=%llu clock_ready=%d activity_owned=%d session_bus=1\n",
           (unsigned long long)start, clock_ready, activity_owned);
    pid_t child = fork();
    if (!child)
    {
        close(ConnectionNumber(display));
        dup2(log, STDOUT_FILENO);
        dup2(log, STDERR_FILENO);
        close(log);
        execv(argv[2], argv + 2);
        perror("STARTUP_EXEC");
        _exit(127);
    }
    close(log);
    unsigned status = 1;
    uint64_t next = start, visible = 0;
    printf("STARTUP_CHILD pid=%ld fork_ms=%llu\n", (long)child,
           (unsigned long long)(milliseconds() - start));
    while (child > 0 && milliseconds() - start < 120000)
    {
        uint64_t now = milliseconds();
        if (now >= next)
        {
            sample(start, &previous, child);
            next = milliseconds() + 1000;
        }
        uint64_t query = milliseconds();
        Window window = browser_window(display, clients, child);
        uint64_t delay = milliseconds() - query;
        if (delay > 100)
            printf("STARTUP_X11_WAIT ms=%llu duration_ms=%llu\n",
                   (unsigned long long)(milliseconds() - start), (unsigned long long)delay);
        if (!visible && window)
        {
            visible = milliseconds();
            printf("STARTUP_WINDOW ms=%llu id=%lu\n", (unsigned long long)(visible - start),
                   window);
        }
        if (visible && milliseconds() - visible >= 3000)
        {
            status = 0;
            break;
        }
        int child_status;
        if (waitpid(child, &child_status, WNOHANG) == child)
        {
            printf("STARTUP_EXIT ms=%llu status=%d\n", (unsigned long long)(milliseconds() - start),
                   child_status);
            child = -1;
            break;
        }
        poll(NULL, 0, 100);
    }
    sample(start, &previous, child);
    if (activity_owned)
        activity(LUME_ACTIVITY_STOP, &work);
    XCloseDisplay(display);
    printf("STARTUP_DONE ms=%llu visible=%d child=%ld status=%u\n",
           (unsigned long long)(milliseconds() - start), visible != 0, (long)child, status);
    return status;
}
