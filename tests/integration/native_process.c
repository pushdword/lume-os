/* SPDX-License-Identifier: BSD-3-Clause */
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

int main(int argc, char **argv)
{
    if (argc < 2)
        return 2;
    pid_t child = fork();
    if (child < 0)
    {
        perror("fork");
        return 1;
    }
    if (!child)
    {
        execv(argv[1], argv + 1);
        perror("execv");
        _exit(127);
    }
    int status;
    pid_t result;
    do
    {
        result = waitpid(child, &status, 0);
    } while (result < 0 && errno == EINTR);
    if (result != child)
    {
        perror("waitpid");
        return 1;
    }
    printf("NATIVE_CHILD_EXIT pid=%ld raw=%d code=%d signal=%d\n", (long)child, status,
           WIFEXITED(status) ? WEXITSTATUS(status) : -1,
           WIFSIGNALED(status) ? WTERMSIG(status) : 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
}
