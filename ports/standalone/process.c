#include "native.h"
#include <lume/process.h>
#include <lume/signal.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern char **environ;

pid_t fork(void)
{
    fflush(NULL);
    return native_result(LUME_SYS_FORK, 0, 0, 0);
}

int execve(const char *path, char *const argv[], char *const envp[])
{
    struct lume_run_request request = {
        .path = (uintptr_t)path, .argv = (uintptr_t)argv, .envp = (uintptr_t)envp};
    while (argv && request.argc < LUME_ARG_MAX && argv[request.argc])
        ++request.argc;
    while (envp && request.envc < LUME_ENV_MAX && envp[request.envc])
        ++request.envc;
    if (!request.argc || request.argc == LUME_ARG_MAX || request.envc == LUME_ENV_MAX)
    {
        errno = E2BIG;
        return -1;
    }
    return native_result(LUME_SYS_EXEC, (uintptr_t)&request, sizeof(request), 0);
}

int execv(const char *path, char *const argv[])
{
    return execve(path, argv, environ);
}

int execvp(const char *file, char *const argv[])
{
    if (strchr(file, '/'))
        return execv(file, argv);
    const char *path = getenv("PATH");
    if (!path)
        path = "/data/bin:/bin:/usr/bin";
    int saved = ENOENT;
    do
    {
        const char *end = strchr(path, ':');
        size_t count = end ? (size_t)(end - path) : strlen(path);
        char name[LUME_PATH_MAX];
        if (snprintf(name, sizeof(name), "%.*s%s%s", (int)count, path, count ? "/" : "", file) >=
            (int)sizeof(name))
        {
            errno = ENAMETOOLONG;
            return -1;
        }
        execv(name, argv);
        if (errno != ENOENT && errno != ENOTDIR)
            saved = errno;
        path = end ? end + 1 : NULL;
    } while (path);
    errno = saved;
    return -1;
}

static int process_group(unsigned operation, pid_t pid, pid_t group)
{
    struct lume_signal_request request = {
        .version = LUME_SIGNAL_VERSION, .bytes = sizeof(request), .pid = pid, .group = group};
    return native_result(LUME_SYS_SIGNAL, operation, (uintptr_t)&request, sizeof(request));
}

int setpgid(pid_t pid, pid_t group)
{
    return process_group(LUME_SIGNAL_SET_GROUP, pid, group);
}

pid_t getpgrp(void)
{
    return process_group(LUME_SIGNAL_GROUP, 0, 0);
}

int kill(pid_t pid, int number)
{
    struct lume_signal_request request = {
        .version = LUME_SIGNAL_VERSION, .bytes = sizeof(request), .pid = pid, .number = number};
    return native_result(LUME_SYS_SIGNAL, LUME_SIGNAL_SEND, (uintptr_t)&request, sizeof(request));
}

int truncate(const char *path, off_t length)
{
    int fd = open(path, O_WRONLY);
    if (fd < 0)
        return -1;
    int result = ftruncate(fd, length);
    int saved = errno;
    int closed = close(fd);
    if (result)
        errno = saved;
    return result ? result : closed;
}
