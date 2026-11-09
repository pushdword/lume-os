/* SPDX-License-Identifier: BSD-3-Clause */
#include <stdio.h>
#include <unistd.h>

#ifndef BENCH_LAUNCH_SCALE
#define BENCH_LAUNCH_SCALE 1
#endif
#ifndef BENCH_LAUNCH_CASE
#define BENCH_LAUNCH_CASE "all"
#endif
#define NUMBER_TEXT_INNER(value) #value
#define NUMBER_TEXT(value) NUMBER_TEXT_INNER(value)

int app_main(int argc, char **argv, char **environment)
{
    (void)argc;
    (void)argv;
    (void)environment;
    char *const arguments[] = {"/usr/bin/osbench",
                               "--case",
                               BENCH_LAUNCH_CASE,
                               "--scale",
                               NUMBER_TEXT(BENCH_LAUNCH_SCALE),
                               NULL};
    execv(arguments[0], arguments);
    perror("osbench execv");
    return 127;
}
