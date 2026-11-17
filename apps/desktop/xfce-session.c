/* SPDX-License-Identifier: BSD-3-Clause */
#include <dbus/dbus.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "lume/abi.h"
#include "lume/syscall_native.h"

static volatile sig_atomic_t stopping;
extern char **environ;

static int user_directory(const char *variable, const char *home, const char *suffix)
{
    char path[128];
    const char *configured = getenv(variable);
    if (configured && configured[0] == '/')
    {
        if (strlen(configured) >= sizeof(path))
            return -1;
        strcpy(path, configured);
    }
    else
    {
        int length = snprintf(path, sizeof(path), "%s/%s", home, suffix);
        if (length < 0 || (size_t)length >= sizeof(path) || setenv(variable, path, 1))
            return -1;
    }
    struct stat metadata;
    return (mkdir(path, 0700) && errno != EEXIST) || stat(path, &metadata) ||
                   !S_ISDIR(metadata.st_mode)
               ? -1
               : 0;
}

static void interrupted(int number)
{
    (void)number;
    stopping = 1;
}

static pid_t launch(const char *path, char *const arguments[])
{
    char installed[256];
    if (!strncmp(path, "/usr/", 5))
    {
        int length = snprintf(installed, sizeof(installed), "/data/%s", path + 5);
        if (length > 0 && (size_t)length < sizeof(installed) && access(installed, X_OK) == 0)
            path = installed;
    }
    pid_t child;
    int result = posix_spawn(&child, path, NULL, NULL, arguments, environ);
    if (result)
    {
        errno = result;
        fprintf(stderr, "xfce: cannot execute %s: %s\n", path, strerror(errno));
        return -1;
    }
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
    kill(*child, SIGTERM);
    for (unsigned i = 0; i < 20 && !exited(child, NULL); ++i)
        poll(NULL, 0, 50);
    if (*child > 0)
    {
        kill(*child, SIGKILL);
        while (waitpid(*child, NULL, 0) < 0 && errno == EINTR)
        {
        }
        *child = -1;
    }
}

static int bus_ready(const char *address, const char *name, pid_t *bus, pid_t *daemon)
{
    for (unsigned i = 0; i < 300 && !stopping; ++i)
    {
        if (exited(bus, NULL) || (daemon && exited(daemon, NULL)))
            break;
        DBusError error;
        dbus_error_init(&error);
        DBusConnection *connection = dbus_connection_open_private(address, &error);
        if (connection)
            dbus_connection_set_exit_on_disconnect(connection, FALSE);
        int ready = connection && dbus_bus_register(connection, &error);
        if (ready && name)
            ready = dbus_bus_name_has_owner(connection, name, &error);
        if (connection)
        {
            dbus_connection_close(connection);
            dbus_connection_unref(connection);
        }
        dbus_error_free(&error);
        if (ready)
            return 0;
        poll(NULL, 0, 100);
    }
    return -1;
}

int main(void)
{
    const char *previous_path = getenv("PATH");
    char search_path[2048];
    int length = snprintf(search_path, sizeof(search_path), "/data/bin:/data/sbin:%s",
                          previous_path ? previous_path : "/usr/bin:/bin:/usr/sbin:/sbin");
    if (length < 0 || (size_t)length >= sizeof(search_path) || setenv("PATH", search_path, 1))
        return 1;
    const char *wayland_display = getenv("WAYLAND_DISPLAY");
    int wayland = wayland_display && *wayland_display;
    const char *home = getenv("HOME");
    if (!home || home[0] != '/' || strlen(home) >= 110)
    {
        fputs("xfce: HOME must name an absolute directory\n", stderr);
        return 1;
    }
    if ((mkdir(home, 0700) && errno != EEXIST) ||
        user_directory("XDG_CONFIG_HOME", home, ".config") ||
        user_directory("XDG_CACHE_HOME", home, ".cache"))
    {
        fputs("xfce: cannot prepare user directories\n", stderr);
        return 1;
    }
    char desktop[128];
    snprintf(desktop, sizeof(desktop), "%s/Desktop", home);
    if (mkdir(desktop, 0755) && errno != EEXIST)
    {
        fprintf(stderr, "xfce: cannot create desktop directory: %s\n", strerror(errno));
        return 1;
    }
    uint64_t nonce;
    if (lume_native_call(LUME_SYS_RANDOM, (uintptr_t)&nonce, sizeof(nonce), 0) < 0)
        return 1;
    char directory[128], socket_path[128], address[160];
    snprintf(directory, sizeof(directory), "/data/tmp/xfce-%lu-%llx", (unsigned long)getuid(),
             (unsigned long long)nonce);
    if (mkdir(directory, 0700))
    {
        fprintf(stderr, "xfce: cannot create runtime directory: %s\n", strerror(errno));
        return 1;
    }
    snprintf(socket_path, sizeof(socket_path), "%s/bus", directory);
    snprintf(address, sizeof(address), "unix:path=%s", socket_path);
    setenv("DBUS_SESSION_BUS_ADDRESS", address, 1);
    if (!wayland)
        setenv("XDG_RUNTIME_DIR", directory, 1);
    else
    {
        const char *runtime = getenv("XDG_RUNTIME_DIR");
        struct stat metadata;
        if (!runtime || runtime[0] != '/' || stat(runtime, &metadata) || !S_ISDIR(metadata.st_mode))
        {
            fputs("xfce: Wayland runtime directory is unavailable\n", stderr);
            rmdir(directory);
            return 1;
        }
        setenv("GDK_BACKEND", "wayland", 1);
        setenv("GDK_GL", "gles", 0);
    }
    setenv("XDG_CONFIG_DIRS", "/data/etc/xdg:/etc/xdg", 1);
    setenv("XDG_DATA_DIRS", "/data/share:/usr/share", 1);
    setenv("XDG_CURRENT_DESKTOP", "XFCE", 1);
    setenv("DESKTOP_SESSION", wayland ? "xfce-wayland" : "xfce", 1);
    setenv("XDG_SESSION_TYPE", wayland ? "wayland" : "x11", 1);
    setenv("XDG_MENU_PREFIX", "xfce-", 1);
    setenv("NO_AT_BRIDGE", "1", 1);
    signal(SIGINT, interrupted);
    signal(SIGTERM, interrupted);
    signal(SIGHUP, interrupted);
    pid_t bus = -1, daemon = -1, session = -1, terminal = -1;
    pid_t panel = -1, settings = -1;
    int result = 1;
    const char *directories[] = {"/data/var", "/data/var/lib", "/data/var/lib/dbus"};
    for (unsigned i = 0; i < sizeof(directories) / sizeof(directories[0]); ++i)
        if (mkdir(directories[i], 0755) && errno != EEXIST)
            goto cleanup;
    char *uuid_arguments[] = {"dbus-uuidgen", "--ensure", NULL};
    session = launch("/usr/libexec/dbus-uuidgen", uuid_arguments);
    int status = 0;
    if (session < 0)
        goto cleanup;
    pid_t waited;
    do
        waited = waitpid(session, &status, 0);
    while (waited < 0 && errno == EINTR && !stopping);
    if (waited != session || !WIFEXITED(status) || WEXITSTATUS(status))
        goto cleanup;
    session = -1;
    char *configuration = access("/data/etc/dbus-1/session.conf", R_OK) == 0
                              ? "--config-file=/data/etc/dbus-1/session.conf"
                              : "--config-file=/etc/dbus-1/session.conf";
    char *bus_arguments[] = {"dbus-daemon", "--nofork", configuration, "--address", address, NULL};
    bus = launch("/usr/libexec/dbus-daemon", bus_arguments);
    if (bus < 0 || bus_ready(address, NULL, &bus, NULL))
    {
        fputs("xfce: session bus did not become ready\n", stderr);
        goto cleanup;
    }
    char *daemon_arguments[] = {"xfconfd", NULL};
    daemon = launch("/usr/libexec/xfconfd", daemon_arguments);
    if (daemon < 0 || bus_ready(address, "org.xfce.Xfconf", &bus, &daemon))
    {
        fputs("xfce: settings service did not become ready\n", stderr);
        goto cleanup;
    }
    if (wayland)
    {
        char *settings_arguments[] = {"xfsettingsd", "--no-daemon", NULL};
        char *panel_arguments[] = {"xfce4-panel", "--disable-wm-check", NULL};
        char *desktop_arguments[] = {"xfdesktop", NULL};
        settings = launch("/usr/bin/xfsettingsd", settings_arguments);
        panel = launch("/usr/bin/xfce4-panel", panel_arguments);
        session = launch("/usr/bin/xfdesktop", desktop_arguments);
        if (settings < 0 || panel < 0 || session < 0 ||
            bus_ready(address, "org.xfce.Panel", &bus, &panel) ||
            bus_ready(address, "org.xfce.xfdesktop", &bus, &session))
        {
            fputs("xfce: Wayland desktop did not become ready\n", stderr);
            goto cleanup;
        }
        printf("XFCE_WAYLAND_READY panel=%ld desktop=%ld bus=%s\n", (long)panel, (long)session,
               address);
        fflush(stdout);
    }
    else
    {
        char *session_arguments[] = {"xfce4-session", "--disable-tcp", NULL};
        session = launch("/usr/bin/xfce4-session", session_arguments);
        if (session < 0)
            goto cleanup;
        char *terminal_arguments[] = {"xterm",   "-title",    "Lume terminal", "-fn",
                                      "9x15",    "-geometry", "72x24+40+40",   "-e",
                                      "/bin/sh", NULL};
        terminal = launch("/usr/bin/xterm", terminal_arguments);
        if (terminal < 0)
            goto cleanup;
    }
    while (!stopping)
    {
        exited(&terminal, NULL);
        int status = 0;
        if (panel > 0 && exited(&panel, &status))
        {
            fprintf(stderr, "xfce: panel exited with status %d\n", status);
            break;
        }
        if (settings > 0)
            exited(&settings, NULL);
        if (exited(&session, &status))
        {
            result = WIFEXITED(status) && !WEXITSTATUS(status) ? 0 : 1;
            break;
        }
        if (exited(&bus, &status))
        {
            fprintf(stderr, "xfce: session bus exited with status %d\n", status);
            break;
        }
        if (daemon > 0 && exited(&daemon, &status) && (!WIFEXITED(status) || WEXITSTATUS(status)))
        {
            fprintf(stderr, "xfce: settings service exited with status %d\n", status);
            break;
        }
        poll(NULL, 0, 100);
    }
    if (stopping)
        result = 0;
cleanup:
    terminate(&terminal);
    terminate(&session);
    terminate(&panel);
    terminate(&settings);
    terminate(&daemon);
    terminate(&bus);
    unlink(socket_path);
    rmdir(directory);
    return result;
}
