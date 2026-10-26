#include <fcntl.h>
#include <spawn.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

struct sample
{
    uint64_t start, end;
    uint32_t hash, iterations;
};

static uint64_t milliseconds(void)
{
    struct timespec time;
    if (clock_gettime(CLOCK_MONOTONIC, &time))
        return 0;
    return (uint64_t)time.tv_sec * 1000 + (unsigned long)time.tv_nsec / 1000000;
}

int main(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "--compute"))
    {
        struct sample result = {.start = milliseconds(), .hash = 2166136261u};
        if (!result.start)
            return 1;
        do
        {
            for (unsigned i = 0; i < 4096; ++i)
                result.hash = (result.hash ^ result.iterations++) * 16777619u;
            result.end = milliseconds();
        } while (result.end - result.start < 200);
        return write(1, &result, sizeof(result)) == sizeof(result) ? 0 : 1;
    }
    if (argc != 2)
        return 2;
    int file = open(argv[1], O_RDONLY), pipefd[2];
    if (file < 0 || pipe(pipefd))
        return 1;
    posix_spawn_file_actions_t actions;
    if (posix_spawn_file_actions_init(&actions) ||
        posix_spawn_file_actions_addclose(&actions, pipefd[0]) ||
        posix_spawn_file_actions_adddup2(&actions, pipefd[1], 1) ||
        posix_spawn_file_actions_addclose(&actions, pipefd[1]))
        return 1;
    uint64_t start = milliseconds();
    char *arguments[] = {argv[0], "--compute", NULL};
    pid_t child;
    if (posix_spawn(&child, argv[0], &actions, NULL, arguments, NULL) ||
        posix_spawn_file_actions_destroy(&actions) || close(pipefd[1]))
        return 1;
    unsigned char buffer[4096];
    unsigned bytes = 0;
    uint32_t hash = 2166136261u;
    while (bytes < 8u * 1024 * 1024)
    {
        ssize_t count = read(file, buffer, sizeof(buffer));
        if (count <= 0 || milliseconds() - start > 30000)
            return 1;
        for (ssize_t i = 0; i < count; ++i)
            hash = (hash ^ buffer[i]) * 16777619u;
        bytes += (unsigned)count;
    }
    uint64_t end = milliseconds();
    struct sample result;
    int status;
    if (read(pipefd[0], &result, sizeof(result)) != sizeof(result) || close(pipefd[0]) ||
        close(file) || waitpid(child, &status, 0) != child || !WIFEXITED(status) ||
        WEXITSTATUS(status) || result.start < start || result.end > end ||
        result.end - result.start < 200 || !result.iterations)
        return 1;
    printf("SD_COMPUTE_PASS bytes=%u read_start=%llu read_end=%llu compute_start=%llu "
           "compute_end=%llu iterations=%u read_hash=%u compute_hash=%u\n",
           bytes, (unsigned long long)start, (unsigned long long)end,
           (unsigned long long)result.start, (unsigned long long)result.end, result.iterations,
           hash, result.hash);
    return 0;
}
