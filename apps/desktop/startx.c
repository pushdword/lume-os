/* SPDX-License-Identifier: BSD-3-Clause */
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "lume/syscall_native.h"
#include "lume/abi.h"
#include "lume/process.h"
#include "lume/render.h"

static volatile sig_atomic_t stopping;
static volatile sig_atomic_t manager_ready;

static pid_t synchronize_clock(void)
{
    struct timespec utc;
    if (!clock_gettime(CLOCK_REALTIME, &utc))
        return -1;
    pid_t child = fork();
    if (!child)
    {
        setpgid(0, 0);
        execlp("clock", "clock", "sync", (char *)NULL);
        _exit(127);
    }
    if (child > 0)
        setpgid(child, child);
    return child;
}

static const char *server_path(void)
{
    const char *fallback = "/usr/bin/Xlume";
    if (access("/usr/bin/Xlume-gpu", X_OK))
        return fallback;
    struct lume_system_features features = {0};
    if (lume_native_call(LUME_SYS_FEATURES, (uintptr_t)&features, sizeof(features),
                         LUME_FEATURE_VERSION) < 0 ||
        !(features.features & LUME_FEATURE_SHARED_WORD_WAIT))
        return fallback;
#if defined(LUME_ARCH_I686)
    uint32_t a = 1, b, c = 0, d;
    __asm__ volatile("cpuid" : "+a"(a), "=b"(b), "+c"(c), "=d"(d));
    if (!(c & 1) || !(features.features & LUME_FEATURE_SIMD_CONTEXT))
        return fallback;
#endif
    int fd = open("/dev/dri/renderD128", O_RDWR | O_CLOEXEC);
    if (fd < 0)
        return fallback;
    struct lume_render_info info = {0};
    struct lume_render_request query = {.version = LUME_RENDER_VERSION,
                                        .bytes = sizeof(query),
                                        .fd = fd,
                                        .data = (uintptr_t)&info,
                                        .length = sizeof(info)};
    int64_t result =
        lume_native_call(LUME_SYS_GPU, LUME_RENDER_INFO, (uintptr_t)&query, sizeof(query));
    close(fd);
    uint64_t needed =
        LUME_RENDER_OPERATION(LUME_RENDER_DISPLAY) | LUME_RENDER_OPERATION(LUME_RENDER_PRESENT) |
        LUME_RENDER_OPERATION(LUME_RENDER_PRIME_IMPORT) |
        LUME_RENDER_OPERATION(LUME_RENDER_PRIME_EXPORT) |
        LUME_RENDER_OPERATION(LUME_RENDER_SUBMIT) | LUME_RENDER_OPERATION(LUME_RENDER_WAIT);
    if (result || !info.backend || info.version != LUME_RENDER_VERSION ||
        info.bytes != sizeof(info) || (info.operations & needed) != needed)
        return fallback;
    return "/usr/bin/Xlume-gpu";
}

static void interrupted(int signal_number)
{
    (void)signal_number;
    stopping = 1;
}

static void manager_started(int signal_number)
{
    (void)signal_number;
    manager_ready = 1;
}

static int write_bytes(int fd, const void *data, size_t bytes)
{
    const char *buffer = data;
    while (bytes)
    {
        ssize_t count = write(fd, buffer, bytes);
        if (count < 0 && errno == EINTR)
            continue;
        if (count <= 0)
            return -1;
        buffer += count;
        bytes -= (size_t)count;
    }
    return 0;
}

static int write_field(int fd, const void *data, unsigned bytes)
{
    unsigned char length[2] = {bytes >> 8, bytes};
    return write_bytes(fd, length, sizeof(length)) || write_bytes(fd, data, bytes);
}

static int authority(char path[128])
{
    if (mkdir("/data/tmp", 0700) && errno != EEXIST)
        return -1;
    struct stat directory;
    if (stat("/data/tmp", &directory))
        return -1;
    if (!S_ISDIR(directory.st_mode))
    {
        errno = ENOTDIR;
        return -1;
    }
    unsigned char random[24], family[2] = {255, 255};
    unsigned char *cookie = random;
    if (lume_native_call(LUME_SYS_RANDOM, (uintptr_t)random, sizeof(random), 0) < 0)
        return -1;
    uint64_t nonce;
    memcpy(&nonce, random + 16, sizeof(nonce));
    snprintf(path, 128, "/data/tmp/xauth-%ld-%llx", (long)getpid(), (unsigned long long)nonce);
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    if (fd < 0)
        return -1;
    int result = write_bytes(fd, family, 2) || write_field(fd, "", 0) || write_field(fd, "0", 1) ||
                 write_field(fd, "MIT-MAGIC-COOKIE-1", 18) || write_field(fd, cookie, 16);
    if (close(fd))
        result = -1;
    memset(random, 0, sizeof(random));
    if (result)
        unlink(path);
    return result ? -1 : 0;
}

static pid_t launch(const char *path, char *const arguments[], int server)
{
    pid_t child = fork();
    if (!child)
    {
        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        signal(SIGHUP, SIG_DFL);
        setpgid(0, 0);
        if (!server)
            lume_native_call(LUME_SYS_CAP_DROP,
                             LUME_CAP_DRIVER_ADMIN | LUME_CAP_HW_DIAGNOSTIC | LUME_CAP_BOOT_UPDATE |
                                 LUME_CAP_SUPERVISOR,
                             0, 0);
        execv(path, arguments);
        fprintf(stderr, "startx: cannot execute %s: %s\n", path, strerror(errno));
        _exit(127);
    }
    if (child > 0)
        setpgid(child, child);
    return child;
}

static int exited(pid_t *child, int *status)
{
    if (*child <= 0)
        return 1;
    pid_t result = waitpid(*child, status, WNOHANG);
    if (result == *child || (result < 0 && errno == ECHILD))
    {
        *child = -1;
        return 1;
    }
    return 0;
}

static void terminate(pid_t *child)
{
    if (*child <= 0)
        return;
    kill(-*child, SIGTERM);
    kill(*child, SIGTERM);
    for (unsigned i = 0; i < 20 && !exited(child, NULL); ++i)
        poll(NULL, 0, 50);
    if (*child > 0)
    {
        kill(-*child, SIGKILL);
        kill(*child, SIGKILL);
        while (waitpid(*child, NULL, 0) < 0 && errno == EINTR)
        {
        }
        *child = -1;
    }
}

static int ready(pid_t *server, pid_t *manager, pid_t *terminal, unsigned attempts)
{
    pid_t probe = fork();
    if (!probe)
    {
        for (;;)
        {
            Display *display = XOpenDisplay("unix/:0");
            if (display)
            {
                int complete = !manager;
                if (manager)
                {
                    Atom type;
                    int format;
                    unsigned long count, remaining;
                    unsigned char *windows = NULL;
                    if (XGetWindowProperty(display, DefaultRootWindow(display),
                                           XInternAtom(display, "_NET_CLIENT_LIST", False), 0, 1,
                                           False, XA_WINDOW, &type, &format, &count, &remaining,
                                           &windows) == Success &&
                        type == XA_WINDOW && format == 32 && count)
                    {
                        XWindowAttributes attributes;
                        complete = XGetWindowAttributes(display, *(Window *)windows, &attributes) &&
                                   attributes.map_state == IsViewable;
                    }
                    XFree(windows);
                }
                XCloseDisplay(display);
                if (complete)
                    _exit(0);
            }
            poll(NULL, 0, 100);
        }
    }
    if (probe < 0)
        return -1;
    int status = 0, result = -1;
    for (unsigned i = 0; i < attempts && !stopping; ++i)
    {
        if (exited(server, NULL))
            break;
        if (manager && (exited(manager, NULL) || (terminal && exited(terminal, NULL))))
            break;
        if (exited(&probe, &status))
        {
            if (!WIFEXITED(status) || WEXITSTATUS(status))
                break;
            if (!manager || manager_ready)
            {
                result = 0;
                break;
            }
        }
        poll(NULL, 0, 100);
    }
    if (probe > 0)
    {
        kill(probe, SIGKILL);
        while (waitpid(probe, NULL, 0) < 0 && errno == EINTR)
        {
        }
    }
    return result;
}

int main(int argc, char **argv)
{
    int xfwm = argc == 2 && !strcmp(argv[1], "xfwm4");
    int xfce = (argc == 2 && !strcmp(argv[1], "xfce")) ||
               (argc == 1 && access("/usr/bin/lume-xfce-session", X_OK) == 0);
    if (argc != 1 && !(argc == 2 && (xfwm || xfce || !strcmp(argv[1], "icewm"))))
    {
        fputs("Usage: startx [icewm|xfwm4|xfce]\n", stderr);
        return 2;
    }
    const char *manager_path = xfce   ? "/usr/bin/lume-xfce-session"
                               : xfwm ? "/usr/bin/xfwm4"
                                      : "/usr/bin/icewm";
    if (access(manager_path, X_OK))
    {
        fprintf(stderr, "startx: desktop is unavailable: %s\n", manager_path);
        return 1;
    }
    char cookie_path[128] = "";
    pid_t server = -1, manager = -1, terminal = -1, clock = -1;
    int result = 1;
    signal(SIGINT, interrupted);
    signal(SIGTERM, interrupted);
    signal(SIGHUP, interrupted);
    signal(SIGUSR1, manager_started);
    signal(SIGUSR2, SIG_IGN);
    if (authority(cookie_path))
    {
        fprintf(stderr, "startx: cannot create authentication file: %s\n", strerror(errno));
        return 1;
    }
    setenv("DISPLAY", "unix/:0", 1);
    setenv("XAUTHORITY", cookie_path, 1);
    setenv("PATH", "/data/bin:/bin:/usr/bin", 1);
    if (!getenv("HOME"))
        setenv("HOME", "/data/home", 1);
    setenv("ICEWM_PRIVCFG", "/etc/icewm", 1);
    setenv("LANG", "C.UTF-8", 1);
    setenv("XMODIFIERS", "@im=none", 1);
    setenv("XAPPLRESDIR", "/usr/share/X11/app-defaults", 1);
    char *server_args[] = {"Xlume", ":0",        "-nolock", "-noreset",
                           "-auth", cookie_path, "-fp",     "/usr/share/fonts/misc,built-ins",
                           "-s",    "0",         NULL};
    clock = synchronize_clock();
    puts("Starting X11 session...");
    const char *selected_server = server_path();
    server = launch(selected_server, server_args, 1);
    int server_ready = server > 0 ? ready(&server, NULL, NULL, 600) : -1;
    if (server_ready && !stopping && strcmp(selected_server, "/usr/bin/Xlume"))
    {
        terminate(&server);
        fputs("startx: accelerated X11 server unavailable\n", stderr);
        selected_server = "/usr/bin/Xlume";
        server = launch(selected_server, server_args, 1);
        server_ready = server > 0 ? ready(&server, NULL, NULL, 600) : -1;
    }
    if (server_ready)
    {
        fputs("startx: X11 server did not become ready\n", stderr);
        goto cleanup;
    }
    int graphics_environment = !strcmp(selected_server, "/usr/bin/Xlume-gpu")
                                   ? setenv("LUME_GL_DISPLAY", getenv("DISPLAY"), 1)
                                   : unsetenv("LUME_GL_DISPLAY");
    if (graphics_environment)
    {
        perror("startx: graphics environment");
        goto cleanup;
    }
    char *manager_args[] = {"icewm", "--notify", "--config=/etc/icewm/preferences", NULL};
    char *xfwm_args[] = {"xfwm4", NULL};
    char *xfce_args[] = {"lume-xfce-session", NULL};
    if (xfwm || xfce)
        manager_ready = 1;
    manager = launch(manager_path, xfce ? xfce_args : xfwm ? xfwm_args : manager_args, 0);
    char *terminal_args[] = {"xterm",       "-title", "Lume terminal", "-fn", "9x15", "-geometry",
                             "72x24+40+40", "-e",     "/bin/sh",       NULL};
    if (!xfce)
        terminal = launch("/usr/bin/xterm", terminal_args, 0);
    if (manager < 0 || (!xfce && terminal < 0))
        goto cleanup;
    if (ready(&server, &manager, xfce ? NULL : &terminal, xfce ? 6000 : xfwm ? 1800 : 600))
    {
        fputs("startx: desktop did not become ready\n", stderr);
        goto cleanup;
    }
    puts("X11 session ready");
    while (!stopping)
    {
        exited(&clock, NULL);
        if (exited(&server, NULL))
        {
            fputs("startx: X11 server exited\n", stderr);
            goto cleanup;
        }
        int manager_status = 0;
        if (exited(&manager, &manager_status))
        {
            result = WIFEXITED(manager_status) && !WEXITSTATUS(manager_status) ? 0 : 1;
            if (result)
                fputs("startx: window manager failed\n", stderr);
            break;
        }
        exited(&terminal, NULL);
        poll(NULL, 0, 100);
    }
cleanup:
    terminate(&clock);
    terminate(&server);
    terminate(&manager);
    terminate(&terminal);
    unlink(cookie_path);
    puts("X11 session ended");
    return result;
}
