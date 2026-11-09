/* SPDX-License-Identifier: BSD-3-Clause */
#include "api.h"
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pty.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

extern int main(int, char **, char **);
static const char *program_name;

const char *getprogname(void)
{
    return program_name ? program_name : "bash";
}

static int interactive(unsigned argc, char **argv)
{
    int force = 0;
    for (unsigned i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "--version") || !strcmp(argv[i], "--help"))
            return 0;
        if (argv[i][0] == '-' && argv[i][1] != '-')
        {
            if (strchr(argv[i] + 1, 'c'))
                return 0;
            if (strchr(argv[i] + 1, 'i'))
                force = 1;
        }
        else if (argv[i][0] != '-')
            return force;
    }
    return 1;
}

static int bridge(int master, pid_t child)
{
    char input[128], output[1024];
    unsigned first = 0, pending = 0;
    int status = 0;
    for (;;)
    {
        int exited = waitpid(child, &status, WNOHANG);
        ssize_t count;
        while ((count = read(master, output, sizeof(output))) > 0)
            if (app_write_all(1, output, (unsigned)count) < 0)
                return 1;
        if (exited == child)
            return WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
        if (exited < 0)
            return 1;
        if (!pending)
        {
            int received = app_read(0, input, sizeof(input));
            if (received > 0)
            {
                first = 0;
                pending = (unsigned)received;
            }
        }
        if (pending)
        {
            count = write(master, input + first, pending);
            if (count > 0)
            {
                first += count;
                pending -= count;
            }
            else if (count < 0 && errno != EAGAIN && errno != EINTR)
                return 1;
        }
        struct pollfd ready = {.fd = master, .events = POLLIN};
        (void)poll(&ready, 1, 10);
    }
}

int app_main(unsigned argc, char **argv, char **envp)
{
    program_name = argc ? argv[0] : "bash";
    const char *name = strrchr(program_name, '/');
    if (name)
        program_name = name + 1;
    struct termios mode;
    if (!interactive(argc, argv) || !isatty(0) || !isatty(2) || !tcgetattr(0, &mode))
        return main((int)argc, argv, envp);
    int master, slave;
    if (openpty(&master, &slave, NULL, NULL, NULL))
        return 1;
    pid_t child = fork();
    if (child < 0)
    {
        close(master);
        close(slave);
        return 1;
    }
    if (!child)
    {
        close(master);
        if (setsid() < 0 || ioctl(slave, TIOCSCTTY, 0) < 0 || dup2(slave, 0) < 0 ||
            dup2(slave, 1) < 0 || dup2(slave, 2) < 0)
            _exit(1);
        if (slave > 2)
            close(slave);
        lume_stdio_reset();
        return main((int)argc, argv, envp);
    }
    close(slave);
    fcntl(master, F_SETFL, O_NONBLOCK);
    int result = bridge(master, child);
    close(master);
    return result;
}
