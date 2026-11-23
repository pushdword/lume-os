/* SPDX-License-Identifier: BSD-3-Clause */
#include "terminal.h"
#include "lume/abi.h"
#include "lume/syscall_native.h"
#include "lume/tty.h"
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

static volatile sig_atomic_t interrupted;
static bool pseudo_terminal;
static bool swallow_line_feed;

static void interrupt_input(int signal)
{
    (void)signal;
    interrupted = 1;
}

static int64_t terminal_name(int fd, struct lume_tty_request *request)
{
    *request =
        (struct lume_tty_request){.version = LUME_TTY_VERSION, .bytes = sizeof(*request), .fd = fd};
    return lume_native_call(LUME_SYS_TTY, LUME_TTY_NAME, (uintptr_t)request, sizeof(*request));
}

static bool attached_console(void)
{
    struct stat console;
    if (stat("/dev/console", &console) || !S_ISCHR(console.st_mode))
        return false;
    for (int fd = STDIN_FILENO; fd <= STDERR_FILENO; ++fd)
    {
        struct stat info;
        if (fstat(fd, &info) || !S_ISCHR(info.st_mode) || info.st_dev != console.st_dev ||
            info.st_ino != console.st_ino)
            return false;
    }
    return true;
}

bool auth_terminal_ready(void)
{
    struct lume_tty_request input, output, errors;
    pseudo_terminal = terminal_name(STDIN_FILENO, &input) == 0;
    if (pseudo_terminal)
    {
        if (input.other != 1 || terminal_name(STDOUT_FILENO, &output) ||
            terminal_name(STDERR_FILENO, &errors) || output.other != 1 || errors.other != 1 ||
            input.value != output.value || input.value != errors.value)
            return false;
    }
    else if (!attached_console())
        return false;
    const int signals[] = {SIGHUP, SIGINT, SIGQUIT, SIGTERM, SIGTSTP};
    struct sigaction action = {.sa_handler = interrupt_input};
    sigemptyset(&action.sa_mask);
    for (size_t i = 0; i < sizeof(signals) / sizeof(signals[0]); ++i)
        if (sigaction(signals[i], &action, NULL))
            return false;
    return true;
}

static bool print_prompt(const char *prompt)
{
    return fputs(prompt, stderr) >= 0 && fflush(stderr) == 0;
}

int auth_read_line(const char *prompt, char *text, size_t capacity, bool echo)
{
    struct termios saved;
    if (pseudo_terminal)
    {
        if (tcgetattr(STDIN_FILENO, &saved))
            return -1;
        struct termios raw = saved;
        cfmakeraw(&raw);
        raw.c_oflag = saved.c_oflag;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &raw))
            return -1;
    }
    int result = -1;
    size_t used = 0;
    bool overflow = false;
    bool prompted = print_prompt(prompt);
    while (prompted && !interrupted)
    {
        unsigned char c;
        ssize_t got = read(STDIN_FILENO, &c, 1);
        if (got < 0 && errno == EINTR)
            continue;
        if (got != 1 || c == 3 || c == 4 || c == 26)
            break;
        bool skip = swallow_line_feed && c == '\n';
        swallow_line_feed = false;
        if (skip)
            continue;
        if (c == '\r' || c == '\n')
        {
            swallow_line_feed = c == '\r';
            if (!overflow)
                result = (int)used;
            break;
        }
        if (c == 8 || c == 127)
        {
            if (used)
            {
                text[--used] = 0;
                if (echo && !print_prompt("\b \b"))
                    break;
            }
            continue;
        }
        if (c < 32)
            continue;
        if (used + 1 == capacity)
        {
            overflow = true;
            continue;
        }
        text[used++] = (char)c;
        if (echo && write(STDERR_FILENO, &c, 1) != 1)
            break;
    }
    text[used] = 0;
    if (pseudo_terminal)
    {
        int status;
        do
            status = tcsetattr(STDIN_FILENO, TCSANOW, &saved);
        while (status && errno == EINTR);
        if (status)
            result = -1;
    }
    if (prompted)
        (void)print_prompt("\n");
    return interrupted ? -1 : result;
}

bool auth_terminal_interrupted(void)
{
    return interrupted != 0;
}

bool auth_terminal_is_pty(void)
{
    return pseudo_terminal;
}
