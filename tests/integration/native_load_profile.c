#define _POSIX_C_SOURCE 200809L
#include "lume/abi.h"
#include "lume/syscall_native.h"
#include <fcntl.h>
#include <spawn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static uint64_t milliseconds(void)
{
    return (uint64_t)lume_native_call(LUME_SYS_CLOCK, 0, 0, 0);
}

int main(int argc, char **argv)
{
    if (argc < 2 || argc > 3)
        return 2;
    int fd = open(argv[1], O_RDONLY);
    struct lume_stat stat;
    if (fd < 0 || lume_native_call(LUME_SYS_STAT, fd, (uintptr_t)&stat, 0) < 0)
        return 3;
    unsigned char *buffer = malloc(65536);
    if (!buffer)
        return 4;
    uint64_t start = milliseconds(), bytes = 0;
    int read_only = argc == 3 && !strcmp(argv[2], "--read-only");
    if (read_only && stat.size > 2097152)
        stat.size = 2097152;
    unsigned sample = 0;
    while (bytes < stat.size)
    {
        unsigned count = stat.size - bytes > 65536 ? 65536 : (unsigned)(stat.size - bytes);
        int64_t received = lume_native_call(LUME_SYS_READ, fd, (uintptr_t)buffer, count);
        if (received <= 0 || (uint64_t)received > count)
            return 5;
        bytes += (uint64_t)received;
        sample = sample * 33u + buffer[0] + buffer[received - 1];
    }
    uint64_t read_ms = milliseconds() - start;
    close(fd);
    free(buffer);
    if (read_only)
    {
        printf("READ_PROFILE bytes=%llu read_ms=%llu sample=%u\n",
               (unsigned long long)bytes, (unsigned long long)read_ms, sample);
        return 0;
    }
    posix_spawn_file_actions_t actions;
    if (posix_spawn_file_actions_init(&actions) ||
        posix_spawn_file_actions_addopen(&actions, 1, "/dev/null", O_WRONLY, 0) ||
        posix_spawn_file_actions_addopen(&actions, 2, "/dev/null", O_WRONLY, 0))
        return 6;
    char *arguments[] = {argv[1], argc == 3 ? argv[2] : "--version", NULL};
    char *environment[] = {"PATH=/usr/bin:/bin:/data/bin", "HOME=/data/home", "LANG=C", NULL};
    pid_t child;
    start = milliseconds();
    int result = posix_spawn(&child, argv[1], &actions, NULL, arguments, environment);
    uint64_t spawned = milliseconds();
    int status = -1;
    if (!result && waitpid(child, &status, 0) != child)
        result = 7;
    uint64_t ended = milliseconds();
    posix_spawn_file_actions_destroy(&actions);
    printf("LOAD_PROFILE result=%d status=%d bytes=%llu read_ms=%llu spawn_ms=%llu run_ms=%llu sample=%u\n",
           result, status, (unsigned long long)bytes, (unsigned long long)read_ms,
           (unsigned long long)(spawned - start), (unsigned long long)(ended - spawned), sample);
    return result || status ? 8 : 0;
}
