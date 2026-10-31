/* SPDX-License-Identifier: BSD-3-Clause */
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pty.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "lume/process.h"
#include "lume/abi.h"
#include "lume/syscall_native.h"
#include <sys/select.h>
#include "fd.h"

static pid_t test_shell;

static void close_test_shell(void)
{
    if (test_shell > 0)
        kill(test_shell, SIGHUP);
}

static void require(int valid, const char *operation)
{
    if (!valid)
    {
        fprintf(stderr, "TERMINAL_FAIL %s errno=%d\n", operation, errno);
        exit(1);
    }
}

static uint64_t milliseconds(void)
{
    struct timespec now;
    require(!clock_gettime(CLOCK_MONOTONIC, &now), "monotonic clock");
    return (uint64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

static void pause_ms(unsigned delay)
{
    require(!poll(NULL, 0, (int)delay), "timed wait");
}

static void reaped(pid_t child)
{
    int status;
    require(waitpid(child, &status, 0) == child && WIFEXITED(status) && !WEXITSTATUS(status),
            "writer exit");
}

static volatile sig_atomic_t interrupted;

static void alarm_handler(int number)
{
    (void)number;
    interrupted = 1;
}

static void terminal_reads(void)
{
    int master, slave;
    require(!openpty(&master, &slave, NULL, NULL, NULL), "create PTY");
    struct termios mode;
    require(!tcgetattr(slave, &mode), "read terminal mode");
    cfmakeraw(&mode);
    mode.c_cc[VMIN] = mode.c_cc[VTIME] = 0;
    require(!tcsetattr(slave, TCSANOW, &mode), "polling mode");
    char bytes[16];
    require(read(slave, bytes, sizeof(bytes)) == 0, "empty immediate read");
    require(write(master, "abc", 3) == 3, "immediate input");
    require(read(slave, bytes, sizeof(bytes)) == 3 && !memcmp(bytes, "abc", 3), "immediate bytes");
    mode.c_cc[VTIME] = 1;
    require(!tcsetattr(slave, TCSANOW, &mode), "initial timeout mode");
    uint64_t start = milliseconds();
    require(read(slave, bytes, sizeof(bytes)) == 0, "initial timeout read");
    require(milliseconds() - start >= 90 && milliseconds() - start < 2000,
            "initial timeout duration");
    mode.c_cc[VMIN] = 4;
    mode.c_cc[VTIME] = 0;
    require(!tcsetattr(slave, TCSANOW, &mode), "minimum byte mode");
    pid_t writer = fork();
    require(writer >= 0, "fork minimum writer");
    if (!writer)
    {
        close(slave);
        pause_ms(30);
        require(write(master, "a", 1) == 1, "first minimum byte");
        pause_ms(50);
        require(write(master, "bcd", 3) == 3, "remaining minimum bytes");
        exit(0);
    }
    require(read(slave, bytes, sizeof(bytes)) == 4 && !memcmp(bytes, "abcd", 4),
            "minimum byte read");
    reaped(writer);
    mode.c_cc[VTIME] = 1;
    require(!tcsetattr(slave, TCSANOW, &mode), "inter-byte mode");
    writer = fork();
    require(writer >= 0, "fork inter-byte writer");
    if (!writer)
    {
        close(slave);
        pause_ms(180);
        require(write(master, "a", 1) == 1, "first timed byte");
        pause_ms(50);
        require(write(master, "b", 1) == 1, "second timed byte");
        exit(0);
    }
    start = milliseconds();
    require(read(slave, bytes, sizeof(bytes)) == 2 && !memcmp(bytes, "ab", 2),
            "inter-byte timeout");
    require(milliseconds() - start >= 310 && milliseconds() - start < 2500, "inter-byte duration");
    reaped(writer);
    require(!fcntl(slave, F_SETFL, O_NONBLOCK), "nonblocking terminal");
    require(read(slave, bytes, sizeof(bytes)) == -1 && errno == EAGAIN, "nonblocking empty read");
    require(write(master, "z", 1) == 1 && read(slave, bytes, sizeof(bytes)) == 1,
            "nonblocking minimum bypass");
    require(!fcntl(slave, F_SETFL, 0), "blocking terminal");
    struct sigaction action = {.sa_handler = alarm_handler};
    require(!sigaction(SIGALRM, &action, NULL), "alarm handler");
    alarm(1);
    require(read(slave, bytes, sizeof(bytes)) == -1 && errno == EINTR && interrupted,
            "interrupt terminal read");
    alarm(0);
    close(master);
    require(read(slave, bytes, sizeof(bytes)) == 0, "terminal hangup");
    close(slave);
    puts("TERMINAL_TIMING_PASS");
}

static void process_states(void)
{
    pid_t child = fork();
    require(child >= 0, "fork process state child");
    if (!child)
    {
        for (;;)
            pause_ms(10);
    }
    require(!setpgid(child, child), "child process group");
    int status;
    require(!kill(-child, SIGSTOP), "stop process group");
    require(waitpid(-child, &status, WUNTRACED) == child && WIFSTOPPED(status) &&
                WSTOPSIG(status) == SIGSTOP,
            "stopped group status");
    require(waitpid(child, &status, WUNTRACED | WNOHANG) == 0, "consume stop notification");
    require(!kill(-child, SIGCONT), "continue process group");
    require(waitpid(-child, &status, WCONTINUED) == child && WIFCONTINUED(status),
            "continued group status");
    require(!kill(child, SIGTSTP), "terminal stop signal");
    require(waitpid(child, &status, WUNTRACED) == child && WIFSTOPPED(status) &&
                WSTOPSIG(status) == SIGTSTP,
            "terminal stopped status");
    require(!kill(child, SIGKILL), "kill stopped process");
    require(waitpid(child, &status, 0) == child && WIFSIGNALED(status) &&
                WTERMSIG(status) == SIGKILL,
            "stopped process exit");
    puts("TERMINAL_PROCESS_STATES_PASS");
}

static void foreground_terminal(void)
{
    int master, slave, ready[2];
    require(!openpty(&master, &slave, NULL, NULL, NULL) && !pipe(ready), "foreground PTY");
    struct termios mode;
    require(!tcgetattr(slave, &mode), "foreground mode");
    cfmakeraw(&mode);
    require(!tcsetattr(slave, TCSANOW, &mode), "foreground raw mode");
    pid_t leader = fork();
    require(leader >= 0, "fork session leader");
    if (!leader)
    {
        close(master);
        close(ready[0]);
        require(setsid() > 0, "new terminal session");
        require(!ioctl(slave, TIOCSCTTY, 0), "controlling terminal");
        require(dup2(slave, 0) == 0, "terminal standard input");
        pid_t background = fork();
        require(background >= 0, "fork background reader");
        if (!background)
        {
            require(!setpgid(0, 0), "reader process group");
            char byte;
            require(read(0, &byte, 1) == 1 && byte == 'q', "foreground resumed input");
            exit(0);
        }
        int status;
        require(waitpid(background, &status, WUNTRACED) == background && WIFSTOPPED(status) &&
                    WSTOPSIG(status) == SIGTTIN,
                "background read stop");
        signal(SIGTTOU, SIG_IGN);
        require(!tcsetpgrp(0, background), "foreground reader group");
        require(write(ready[1], "r", 1) == 1, "foreground input readiness");
        require(!kill(-background, SIGCONT), "resume foreground reader");
        reaped(background);
        require(!tcsetpgrp(0, getpgrp()), "restore terminal group");
        exit(0);
    }
    close(slave);
    close(ready[1]);
    char byte;
    require(read(ready[0], &byte, 1) == 1 && byte == 'r', "foreground transfer barrier");
    require(write(master, "q", 1) == 1, "foreground keyboard byte");
    reaped(leader);
    close(master);
    close(ready[0]);
    puts("TERMINAL_FOREGROUND_PASS");
}

static void shell_expect(int master, const char *text)
{
    char output[8192];
    unsigned used = 0;
    uint64_t deadline = milliseconds() + 5000;
    output[0] = 0;
    while (!strstr(output, text) && milliseconds() < deadline)
    {
        struct pollfd ready = {.fd = master, .events = POLLIN};
        int result = poll(&ready, 1, 100);
        require(result >= 0, "shell output wait");
        if (!result)
            continue;
        int bytes = read(master, output + used, sizeof(output) - used - 1);
        require(bytes > 0, "shell output");
        used += (unsigned)bytes;
        output[used] = 0;
        require(used + 1 < sizeof(output), "shell output bound");
    }
    if (!strstr(output, text))
        fprintf(stderr, "SHELL_OUTPUT %s expected=%s\n", output, text);
    require(strstr(output, text) != NULL, "shell expected output");
}

static void shell_input(int master, const char *text)
{
    size_t bytes = strlen(text);
    require(write(master, text, bytes) == (ssize_t)bytes, "shell keyboard input");
}

static void shell_ready(int master, pid_t shell, unsigned children)
{
    uint64_t deadline = milliseconds() + 5000;
    unsigned last_ready = 0;
    do
    {
        unsigned ready = 0;
        for (unsigned i = 0; i < 128; ++i)
        {
            struct lume_task_info task;
            if (lume_native_call(LUME_SYS_PROCESS, i, (uintptr_t)&task, sizeof(task)) <= 0)
                continue;
            if (task.parent == (unsigned)shell && !strcmp(task.name, "cat") &&
                task.state != LUME_PROCESS_STOPPED)
                ++ready;
        }
        if (ready == children && tcgetpgrp(master) != shell)
            return;
        last_ready = ready;
        pause_ms(20);
    } while (milliseconds() < deadline);
    fprintf(stderr, "PIPELINE_READY shell=%ld expected=%u observed=%u foreground=%ld\n",
            (long)shell, children, last_ready, (long)tcgetpgrp(master));
    for (unsigned i = 0; i < 128; ++i)
    {
        struct lume_task_info task;
        if (lume_native_call(LUME_SYS_PROCESS, i, (uintptr_t)&task, sizeof(task)) <= 0)
            continue;
        fprintf(stderr, "PIPELINE_TASK pid=%u parent=%u state=%u name=%s\n", task.pid, task.parent,
                task.state, task.name);
    }
    require(0, "foreground pipeline readiness");
}

static void shell_stopped(pid_t shell, unsigned children)
{
    uint64_t deadline = milliseconds() + 5000;
    unsigned stopped = 0;
    do
    {
        stopped = 0;
        for (unsigned i = 0; i < 128; ++i)
        {
            struct lume_task_info task;
            if (lume_native_call(LUME_SYS_PROCESS, i, (uintptr_t)&task, sizeof(task)) <= 0)
                continue;
            if (task.parent == (unsigned)shell && !strcmp(task.name, "cat") &&
                task.state == LUME_PROCESS_STOPPED)
                ++stopped;
        }
        if (stopped == children)
            return;
        pause_ms(20);
    } while (milliseconds() < deadline);
    fprintf(stderr, "PIPELINE_STOPPED expected=%u observed=%u\n", children, stopped);
    require(0, "background pipeline stop readiness");
}

static void shell_jobs(void)
{
    int master, slave;
    require(!openpty(&master, &slave, NULL, NULL, NULL), "shell PTY");
    pid_t shell = fork();
    require(shell >= 0, "fork interactive shell");
    if (!shell)
    {
        close(master);
        require(setsid() > 0 && !ioctl(slave, TIOCSCTTY, 0), "shell controlling terminal");
        for (int fd = 0; fd < 3; ++fd)
            require(dup2(slave, fd) == fd, "shell standard descriptors");
        close(slave);
        execl("/bin/sh", "sh", NULL);
        exit(127);
    }
    test_shell = shell;
    close(slave);
    shell_expect(master, "# ");
    shell_input(master, "cat\n");
    shell_ready(master, shell, 1);
    shell_input(master, "\032");
    shell_expect(master, "# ");
    shell_input(master, "jobs\n");
    shell_expect(master, "Stopped cat");
    pause_ms(30);
    shell_input(master, "fg\n");
    pause_ms(60);
    shell_input(master, "resumed\n");
    shell_expect(master, "resumed");
    shell_input(master, "\003");
    shell_expect(master, "# ");
    shell_input(master, "cat | cat\n");
    shell_ready(master, shell, 2);
    shell_input(master, "\032");
    shell_expect(master, "# ");
    shell_input(master, "bg\n");
    shell_expect(master, "# ");
    shell_stopped(shell, 2);
    shell_input(master, "jobs\n");
    shell_expect(master, "Stopped cat | cat");
    pause_ms(30);
    shell_input(master, "fg\n");
    pause_ms(60);
    shell_input(master, "\003");
    shell_expect(master, "# ");
    shell_input(master, "echo background &\n");
    shell_expect(master, "# ");
    unlink("/data/tmp/terminal-utf8");
    shell_input(master, "/usr/bin/vi -u NONE -i NONE -n /data/tmp/terminal-utf8\n");
    struct termios vim_mode;
    uint64_t ready_deadline = milliseconds() + 5000;
    do
    {
        require(!tcgetattr(master, &vim_mode), "Vim terminal mode");
        bool editor = false;
        pid_t group = tcgetpgrp(master);
        for (unsigned i = 0; i < 128; ++i)
        {
            struct lume_task_info task;
            if (lume_native_call(LUME_SYS_PROCESS, i, (uintptr_t)&task, sizeof(task)) <= 0)
                continue;
            if (task.parent == (unsigned)shell && task.pid == (unsigned)group &&
                !strcmp(task.name, "vi"))
                editor = true;
        }
        if (editor && !(vim_mode.c_lflag & ICANON))
            break;
        pause_ms(20);
    } while (milliseconds() < ready_deadline);
    require(milliseconds() < ready_deadline && !(vim_mode.c_lflag & ICANON),
            "Vim foreground raw input mode");
    shell_input(master, "ia\xc3\xa7\xc3\xa3o\xc3\xa9\177, portugu\xc3\xaas, \xe2\x82\xac");
    shell_input(master, "\033");
    pause_ms(60);
    shell_input(master, ":wq\n");
    ready_deadline = milliseconds() + 5000;
    while (tcgetpgrp(master) != shell && milliseconds() < ready_deadline)
        pause_ms(20);
    require(tcgetpgrp(master) == shell, "Vim foreground release");
    shell_expect(master, "# ");
    int file = open("/data/tmp/terminal-utf8", O_RDONLY);
    char content[128];
    const char expected[] = "a\xc3\xa7\xc3\xa3o, portugu\xc3\xaas, \xe2\x82\xac\n";
    require(file >= 0, "Vim UTF-8 file");
    int bytes = read(file, content, sizeof(content));
    if (bytes != sizeof(expected) - 1 || memcmp(content, expected, sizeof(expected) - 1))
    {
        printf("VIM_BYTES count=%d local=%u:", bytes, vim_mode.c_lflag);
        for (int i = 0; i < bytes; ++i)
            printf(" %02x", (unsigned char)content[i]);
        puts("");
    }
    require(bytes == sizeof(expected) - 1 && !memcmp(content, expected, sizeof(expected) - 1),
            "Vim multibyte editing");
    close(file);
    puts("TERMINAL_VIM_UTF8_PASS");
    shell_input(master, "exit\n");
    reaped(shell);
    test_shell = 0;
    close(master);
    puts("TERMINAL_SHELL_JOBS_PASS");
}

int main(void)
{
    require(!atexit(close_test_shell), "test cleanup");
    require(!mkdir("/data/tmp", 0777) || errno == EEXIST, "temporary directory");
    int descriptors[2];
    require(!pipe(descriptors), "select pipe");
    int high = LUME_FD_LIMIT - 1;
    require(dup2(descriptors[0], high) == high, "high descriptor");
    require(getdtablesize() == LUME_FD_LIMIT, "descriptor capacity");
    require(write(descriptors[1], "x", 1) == 1, "select payload");
    fd_set ready;
    FD_ZERO(&ready);
    FD_SET(high, &ready);
    struct timeval timeout = {0};
    require(select(high + 1, &ready, NULL, NULL, &timeout) == 1 && FD_ISSET(high, &ready),
            "high descriptor select");
    char byte;
    require(read(high, &byte, 1) == 1 && byte == 'x', "selected payload");
    FD_ZERO(&ready);
    FD_SET(high, &ready);
    require(select(high + 1, &ready, NULL, NULL, &timeout) == 0 && !FD_ISSET(high, &ready),
            "high descriptor timeout");
    close(high);
    close(descriptors[0]);
    close(descriptors[1]);
    puts("TERMINAL_SELECT_CAPACITY_PASS");
    terminal_reads();
    process_states();
    foreground_terminal();
    shell_jobs();
    puts("TERMINAL_NATIVE_PASS");
    return 0;
}
