/* SPDX-License-Identifier: BSD-3-Clause */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

extern char **environ;
static const char *const commands[] = {
#include "commands.h"
};

int main(int argc, char **argv)
{
    if (argc == 1 || (argc == 2 && !strcmp(argv[1], "--help")))
    {
        puts("Usage: coreutils COMMAND [ARG]...");
        for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); ++i)
        {
            puts(commands[i]);
        }
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "--version"))
    {
        puts("Lume command launcher for GNU coreutils 9.12");
        return 0;
    }
    for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); ++i)
    {
        if (strcmp(argv[1], commands[i]))
        {
            continue;
        }
        char path[128];
        int length = snprintf(path, sizeof(path), "/data/libexec/coreutils/%s", commands[i]);
        if (length < 0 || (size_t)length >= sizeof(path))
        {
            fputs("coreutils: command path is too long\n", stderr);
            return 126;
        }
        execve(path, argv + 1, environ);
        int error = errno;
        fprintf(stderr, "coreutils: %s: %s\n", commands[i], strerror(error));
        return error == ENOENT ? 127 : 126;
    }
    fprintf(stderr, "coreutils: unknown command: %s\n", argv[1]);
    return 127;
}
