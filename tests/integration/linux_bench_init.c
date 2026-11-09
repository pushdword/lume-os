/* SPDX-License-Identifier: BSD-3-Clause */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mount.h>
#include <unistd.h>

#ifndef BENCH_INIT_SCALE
#define BENCH_INIT_SCALE 0
#endif
#ifndef BENCH_INIT_CASE
#define BENCH_INIT_CASE 0
#endif

_Static_assert(BENCH_INIT_SCALE >= 0 && BENCH_INIT_SCALE <= 64, "work scale bounds");

static int environment_error(const char *stage, int error)
{
    char message[128];
    int count = snprintf(message, sizeof(message), "BENCH_ENV,error,%s,%d\n", stage, error);
    if (count > 0 && (unsigned)count < sizeof(message))
        (void)write(STDERR_FILENO, message, (size_t)count);
    return 1;
}

int main(int argc, char **argv)
{
    int log = open("/dev/kmsg", O_WRONLY);
    if (log < 0)
        return environment_error("kmsg_open", errno);
    if (dup2(log, STDOUT_FILENO) < 0 || dup2(log, STDERR_FILENO) < 0)
        return environment_error("stdio_redirect", errno);
    if (log > STDERR_FILENO && close(log))
        return environment_error("kmsg_close", errno);
    if (mount("proc", "/proc", "proc", 0, 0))
        return environment_error("proc_mount", errno);
    int setting = open("/proc/sys/kernel/printk_devkmsg", O_WRONLY);
    if (setting < 0)
        return environment_error("devkmsg_setting_open", errno);
    ssize_t written;
    do
        written = write(setting, "on\n", 3);
    while (written < 0 && errno == EINTR);
    int error = written < 0 ? errno : EIO;
    if (written != 3)
    {
        (void)close(setting);
        return environment_error("devkmsg_setting_write", error);
    }
    if (close(setting))
        return environment_error("devkmsg_setting_close", errno);
    if (argc > 1)
    {
        argv[0] = "/bench";
        execv(argv[0], argv);
    }
    else
    {
        char scale[24];
        char *arguments[6] = {"/bench", 0};
        unsigned count = 1;
        if (BENCH_INIT_SCALE)
        {
            (void)snprintf(scale, sizeof(scale), "%u", (unsigned)BENCH_INIT_SCALE);
            arguments[count++] = "--scale";
            arguments[count++] = scale;
        }
        const char *selection = BENCH_INIT_CASE;
        if (selection)
        {
            arguments[count++] = "--case";
            arguments[count++] = (char *)selection;
        }
        arguments[count] = 0;
        execv(arguments[0], arguments);
    }
    return environment_error("bench_exec", errno);
}
