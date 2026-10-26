#include <lume/abi.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static struct
{
    FILE *file;
    pid_t pid;
} streams[LUME_FD_MAX];

FILE *popen(const char *command, const char *mode)
{
    if (!mode || (strcmp(mode, "r") && strcmp(mode, "w")))
    {
        errno = EINVAL;
        return NULL;
    }
    unsigned slot;
    for (slot = 0; slot < LUME_FD_MAX && streams[slot].file; ++slot)
    {
    }
    if (slot == LUME_FD_MAX)
    {
        errno = EMFILE;
        return NULL;
    }
    int fds[2];
    if (pipe(fds))
        return NULL;
    int parent = mode[0] == 'r' ? 0 : 1;
    int child = 1 - parent;
    pid_t pid = fork();
    if (!pid)
    {
        close(fds[parent]);
        if (fds[child] != child)
        {
            if (dup2(fds[child], child) < 0)
                _exit(126);
            close(fds[child]);
        }
        for (unsigned i = 0; i < LUME_FD_MAX; ++i)
            if (streams[i].file)
                close(fileno(streams[i].file));
        char *args[] = {"/bin/sh", "-c", (char *)command, NULL};
        execv(args[0], args);
        _exit(127);
    }
    int saved = errno;
    close(fds[child]);
    if (pid < 0)
    {
        close(fds[parent]);
        errno = saved;
        return NULL;
    }
    FILE *file = fdopen(fds[parent], mode);
    if (!file)
    {
        saved = errno;
        close(fds[parent]);
        while (waitpid(pid, NULL, 0) < 0 && errno == EINTR)
        {
        }
        errno = saved;
        return NULL;
    }
    streams[slot].file = file;
    streams[slot].pid = pid;
    return file;
}

int pclose(FILE *file)
{
    unsigned slot;
    for (slot = 0; slot < LUME_FD_MAX && streams[slot].file != file; ++slot)
    {
    }
    if (!file || slot == LUME_FD_MAX)
    {
        errno = EINVAL;
        return -1;
    }
    pid_t pid = streams[slot].pid;
    streams[slot].file = NULL;
    int closed = fclose(file);
    int saved = errno;
    int status;
    while (waitpid(pid, &status, 0) < 0)
        if (errno != EINTR)
            return -1;
    if (closed)
    {
        errno = saved;
        return -1;
    }
    return status;
}
