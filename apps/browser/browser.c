/* SPDX-License-Identifier: BSD-3-Clause */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    const char *home = getenv("HOME");
    if (home && *home && mkdir(home, 0700) && errno != EEXIST)
        perror("browser: home");
    struct timespec utc;
    if (clock_gettime(CLOCK_REALTIME, &utc))
    {
        pid_t child = fork();
        if (!child)
        {
            execlp("clock", "clock", "sync", (char *)NULL);
            _exit(127);
        }
        if (child > 0)
        {
            int status;
            while (waitpid(child, &status, 0) < 0 && errno == EINTR)
            {
            }
        }
    }
    if (mkdir("/data/downloads", 0700) && errno != EEXIST)
        perror("browser: downloads");
    argv[0] = "dillo";
    (void)argc;
    execvp("dillo", argv);
    perror("browser: dillo");
    return 1;
}
