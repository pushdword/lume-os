/* SPDX-License-Identifier: BSD-3-Clause */
#include <X11/Xlib.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "lume/abi.h"
#include "lume/syscall_native.h"
#include "browser_gpu_page.h"

static volatile sig_atomic_t stopping;
static const char authority_path[] = "/data/tmp/browser-gpu.xauth";
static const char page_path[] = "/data/gpu-probe.html";

static void interrupted(int number)
{
    (void)number;
    stopping = 1;
}

static void terminate(pid_t child)
{
    if (child <= 0)
        return;
    kill(-child, SIGTERM);
    kill(child, SIGTERM);
    for (unsigned i = 0; i < 20; ++i)
    {
        pid_t result = waitpid(child, NULL, WNOHANG);
        if (result == child || (result < 0 && errno == ECHILD))
            return;
        poll(NULL, 0, 50);
    }
    kill(-child, SIGKILL);
    kill(child, SIGKILL);
    while (waitpid(child, NULL, 0) < 0 && errno == EINTR)
    {
    }
}

static pid_t launch(const char *path, char *const arguments[])
{
    pid_t child = fork();
    if (!child)
    {
        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        signal(SIGHUP, SIG_DFL);
        setpgid(0, 0);
        execv(path, arguments);
        perror(path);
        _exit(127);
    }
    if (child > 0)
        setpgid(child, child);
    return child;
}

static int write_file(const char *path, const void *input, size_t bytes)
{
    const unsigned char *data = input;
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    if (fd < 0)
        return -1;
    size_t offset = 0;
    while (offset < bytes)
    {
        ssize_t count = write(fd, data + offset, bytes - offset);
        if (count < 0 && errno == EINTR)
            continue;
        if (count <= 0)
            break;
        offset += (size_t)count;
    }
    int result = close(fd);
    if (offset != bytes || result)
    {
        unlink(path);
        return -1;
    }
    return 0;
}

static int create_authority(void)
{
    unsigned char data[45] = {255, 255, 0, 0, 0, 1, '0', 0, 18};
    memcpy(data + 9, "MIT-MAGIC-COOKIE-1", 18);
    data[28] = 16;
    if (lume_native_call(LUME_SYS_RANDOM, (uintptr_t)(data + 29), 16, 0) < 0)
        return -1;
    int result = write_file(authority_path, data, sizeof(data));
    memset(data, 0, sizeof(data));
    return result;
}

static int prepare(void)
{
    const char *directories[] = {"/data/tmp", "/data/home", "/data/chromium-gpu-profile"};
    for (unsigned i = 0; i < sizeof(directories) / sizeof(directories[0]); ++i)
        if (mkdir(directories[i], 0700) && errno != EEXIST)
            return -1;
    if (setenv("DISPLAY", "unix/:0", 1) || setenv("HOME", "/data/home", 1) ||
        setenv("TMPDIR", "/data/tmp", 1) || setenv("XDG_RUNTIME_DIR", "/data/tmp", 1) ||
        setenv("XAUTHORITY", authority_path, 1) ||
        setenv("FONTCONFIG_FILE", "/data/libexec/chromium/fonts.conf", 1) ||
        setenv("FONTCONFIG_PATH", "/data/libexec/chromium", 1))
        return -1;
    if (!XInitThreads())
        return -1;
    if (write_file(page_path, browser_gpu_page, sizeof(browser_gpu_page) - 1))
        return -1;
    if (create_authority())
    {
        unlink(page_path);
        return -1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    int interactive = argc == 2 && !strcmp(argv[1], "--interactive");
    if (argc != 1 && !interactive)
    {
        fputs("Usage: browser-gpu-probe [--interactive]\n", stderr);
        return 2;
    }
    signal(SIGINT, interrupted);
    signal(SIGTERM, interrupted);
    signal(SIGHUP, interrupted);
    if (prepare())
    {
        perror("FAIL_CHROMIUM_GPU_SETUP");
        return 1;
    }
    char *server_arguments[] = {"Xlume-gpu", ":0",       "-auth", (char *)authority_path,
                                "-nolock",   "-noreset", "-fp",   "built-ins",
                                "-s",        "0",        NULL};
    pid_t server = launch("/usr/bin/Xlume-gpu", server_arguments);
    pid_t browser = -1;
    int result = 1;
    if (server < 0)
        goto cleanup;
    unsigned attempts;
    for (attempts = 0; attempts < 200 && !stopping; ++attempts)
    {
        if (waitpid(server, NULL, WNOHANG) == server)
        {
            server = -1;
            break;
        }
        Display *display = XOpenDisplay("unix/:0");
        if (display)
        {
            XCloseDisplay(display);
            break;
        }
        poll(NULL, 0, 50);
    }
    if (server <= 0 || attempts == 200 || stopping)
    {
        fputs("FAIL_CHROMIUM_GPU_X11_READY\n", stderr);
        goto cleanup;
    }
    if (setenv("LUME_GL_DISPLAY", getenv("DISPLAY"), 1))
    {
        perror("FAIL_CHROMIUM_GPU_SESSION");
        goto cleanup;
    }
    char *browser_arguments[] = {"browser",
                                 "--disable-software-rasterizer",
                                 "--no-first-run",
                                 "--disable-default-apps",
                                 "--disable-background-networking",
                                 "--disable-component-update",
                                 "--disable-sync",
                                 "--disable-dev-shm-usage",
                                 "--enable-logging=stderr",
                                 "--log-level=0",
                                 "--user-data-dir=/data/chromium-gpu-profile",
                                 interactive ? "file:///data/gpu-probe.html"
                                             : "--app=file:///data/gpu-probe.html",
                                 NULL};
    puts("CHROMIUM_GPU_START");
    fflush(stdout);
    browser = launch("/data/bin/browser", browser_arguments);
    if (browser <= 0)
        goto cleanup;
    for (unsigned i = 0; !stopping && (interactive || i < 6000); ++i)
    {
        int status;
        if (waitpid(browser, &status, WNOHANG) == browser)
        {
            browser = -1;
            if (interactive && WIFEXITED(status))
            {
                result = WEXITSTATUS(status);
                goto cleanup;
            }
            fprintf(stderr, "FAIL_CHROMIUM_GPU_BROWSER_EXIT status=%d\n", status);
            goto cleanup;
        }
        if (waitpid(server, &status, WNOHANG) == server)
        {
            server = -1;
            fputs("FAIL_CHROMIUM_GPU_SERVER_EXIT\n", stderr);
            goto cleanup;
        }
        poll(NULL, 0, 50);
    }
    if (!stopping)
        fputs("FAIL_CHROMIUM_GPU_TIMEOUT\n", stderr);
    else
        result = 0;
cleanup:
    terminate(browser);
    terminate(server);
    unlink(authority_path);
    unlink(page_path);
    return result;
}
