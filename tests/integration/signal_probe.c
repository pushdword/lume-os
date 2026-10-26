#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <poll.h>
#include <stdlib.h>
#include <stdint.h>

static volatile sig_atomic_t delivered, alarms, children;

static void handler(int number)
{
    if (number == SIGUSR1)
        ++delivered;
    else if (number == SIGALRM)
        ++alarms;
    else if (number == SIGCHLD)
        ++children;
}

static int check(int condition, int line)
{
    if (!condition)
        fprintf(stderr, "SIGNAL_FAIL line=%d errno=%d\n", line, errno);
    return condition;
}

#define REQUIRE(x)                                                                                 \
    do                                                                                             \
    {                                                                                              \
        if (!check((x), __LINE__))                                                                 \
            return 1;                                                                              \
    } while (0)

int main(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "exec"))
    {
        struct sigaction action;
        REQUIRE(!sigaction(SIGUSR1, NULL, &action) && action.sa_handler == SIG_DFL);
        REQUIRE(!sigaction(SIGUSR2, NULL, &action) && action.sa_handler == SIG_IGN);
        puts("SIGNAL_EXEC_PASS");
        return 23;
    }
    struct sigaction action = {.sa_handler = handler};
    sigemptyset(&action.sa_mask);
    REQUIRE(!sigaction(SIGUSR1, &action, NULL));
    REQUIRE(!sigaction(SIGALRM, &action, NULL));
    REQUIRE(!sigaction(SIGCHLD, &action, NULL));
    struct sigaction ignored = {.sa_handler = SIG_IGN};
    REQUIRE(!sigaction(SIGUSR2, &ignored, NULL));
    REQUIRE(!seteuid(0) && !setegid(0) && !getuid() && !geteuid() && !getgid() && !getegid());
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    REQUIRE(!sigprocmask(SIG_BLOCK, &mask, NULL));
    REQUIRE(!kill(getpid(), SIGUSR1) && delivered == 0);
    REQUIRE(!sigprocmask(SIG_UNBLOCK, &mask, NULL) && delivered == 1);
    sigset_t empty, current;
    sigemptyset(&empty);
    REQUIRE(!sigprocmask(SIG_BLOCK, &mask, NULL));
    for (unsigned i = 0; i < 20; ++i)
    {
        REQUIRE(!kill(getpid(), SIGUSR1));
        struct timespec timeout = {.tv_sec = 1};
        REQUIRE(ppoll(NULL, 0, &timeout, &empty) == -1 && errno == EINTR);
        REQUIRE(!sigprocmask(SIG_SETMASK, NULL, &current) && sigismember(&current, SIGUSR1));
    }
    REQUIRE(delivered == 21);
    REQUIRE(!sigprocmask(SIG_UNBLOCK, &mask, NULL));
    struct itimerval timer = {.it_value.tv_usec = 40000};
    REQUIRE(!setitimer(ITIMER_REAL, &timer, NULL));
    struct timespec delay = {.tv_sec = 1};
    REQUIRE(nanosleep(&delay, NULL) == -1 && errno == EINTR && alarms == 1);
    pid_t parent = getpid(), child = fork();
    REQUIRE(child >= 0);
    if (!child)
    {
        REQUIRE(getppid() == parent);
        REQUIRE(setsid() == getpid() && getpgrp() == getpid() && getsid(0) == getpid());
        usleep(30000);
        REQUIRE(!kill(parent, SIGUSR1));
        usleep(30000);
        execl(argv[0], argv[0], "exec", (char *)NULL);
        return 2;
    }
    int status, got;
    do
    {
        got = waitpid(child, &status, 0);
    } while (got < 0 && errno == EINTR);
    REQUIRE(got == child && WIFEXITED(status) && WEXITSTATUS(status) == 23);
    REQUIRE(delivered == 22 && children >= 1);
    child = fork();
    REQUIRE(child >= 0);
    if (!child)
    {
        for (;;)
            usleep(10000);
    }
    REQUIRE(!kill(child, SIGTERM));
    do
    {
        got = waitpid(child, &status, 0);
    } while (got < 0 && errno == EINTR);
    REQUIRE(got == child && WIFSIGNALED(status) && WTERMSIG(status) == SIGTERM);
    REQUIRE(!kill(getpid(), 0));
    REQUIRE(kill(2000, 0) == -1 && errno == ESRCH);
    REQUIRE(kill((pid_t)INT32_MIN, 0) == -1 && errno == ESRCH);
    FILE *file = fopen("/tmp/ssh-lines", "w+");
    REQUIRE(file != NULL);
    for (unsigned i = 0; i < 129; ++i)
        REQUIRE(fputc('a', file) == 'a');
    REQUIRE(fputs("\n\ntail", file) >= 0 && !fseek(file, 0, SEEK_SET));
    char *line = NULL;
    size_t capacity = 0;
    REQUIRE(getline(&line, &capacity, file) == 130 && line[130] == 0 && line[129] == '\n');
    REQUIRE(getline(&line, &capacity, file) == 1 && !strcmp(line, "\n"));
    REQUIRE(getline(&line, &capacity, file) == 4 && !strcmp(line, "tail"));
    REQUIRE(getline(&line, &capacity, file) == -1);
    free(line);
    REQUIRE(!fclose(file));
    REQUIRE(!unlink("/tmp/ssh-lines"));
    puts("SIGNAL_ALARM_FORK_EXEC_WAIT_PASS");
    return 0;
}
