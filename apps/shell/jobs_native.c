/* SPDX-License-Identifier: BSD-3-Clause */
#include "jobs.h"
#include "lume/syscall_native.h"
#include "lume/signal_native.h"
#include "lume/abi.h"
#include "lume/process.h"
#include "lume/signal.h"
#include "lume/tty.h"
#include "fd.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JOB_LIMIT 16u
#define JOB_PROCESSES 6u

static struct shell_job
{
    unsigned id, group, count, live, stopped, result;
    int pids[JOB_PROCESSES];
    char command[128];
    struct lume_termios mode;
    int background, mode_saved;
} jobs[JOB_LIMIT], pending, *current;

static unsigned next_id = 1, shell_group;
static int monitor, terminal_fd = -1;
static struct lume_termios shell_mode;

static int signal_request(unsigned operation, struct lume_signal_request *request)
{
    request->version = LUME_SIGNAL_VERSION;
    request->bytes = sizeof(*request);
    return (int)lume_native_call(LUME_SYS_SIGNAL, operation, (uintptr_t)request, sizeof(*request));
}

static void disposition(unsigned number, uintptr_t handler)
{
    struct lume_signal_request request = {.number = number,
                                          .operation = 1,
                                          .handler = handler,
                                          .restorer = handler > 1 ? (uintptr_t)signal_restorer : 0};
    (void)signal_request(LUME_SIGNAL_ACTION, &request);
}

static int send_signal(unsigned group, unsigned number)
{
    struct lume_signal_request request = {.pid = -(int32_t)group, .number = number};
    return signal_request(LUME_SIGNAL_SEND, &request);
}

static int terminal(unsigned action, unsigned value, struct lume_termios *mode)
{
    struct lume_tty_request request = {
        .version = LUME_TTY_VERSION, .bytes = sizeof(request), .fd = terminal_fd, .value = value};
    if (mode)
        request.termios = *mode;
    int result = (int)lume_native_call(LUME_SYS_TTY, action, (uintptr_t)&request, sizeof(request));
    if (!result && mode && action == LUME_TTY_GET)
        *mode = request.termios;
    return result;
}

void shell_jobs_shutdown(void)
{
    for (unsigned i = 0; i < JOB_LIMIT; ++i)
        if (jobs[i].id && jobs[i].live)
        {
            (void)send_signal(jobs[i].group, 1);
            (void)send_signal(jobs[i].group, 19);
        }
}

static void hangup(int number)
{
    shell_jobs_shutdown();
    lume_native_call(LUME_SYS_EXIT, 128u + (unsigned)number, 0, 0);
    for (;;)
    {
    }
}

void shell_jobs_initialize(int interactive)
{
    struct lume_signal_request request = {0};
    shell_group = (unsigned)signal_request(LUME_SIGNAL_GROUP, &request);
    struct lume_fd_request duplicate = {.version = LUME_FD_VERSION,
                                        .bytes = sizeof(duplicate),
                                        .fd = 0,
                                        .other = 3,
                                        .flags = LUME_FD_CLOEXEC};
    terminal_fd =
        (int)lume_native_call(LUME_SYS_FD, LUME_FD_DUP, (uintptr_t)&duplicate, sizeof(duplicate));
    monitor = interactive && !terminal(LUME_TTY_GET, 0, &shell_mode);
    if (monitor)
    {
        const unsigned ignored[] = {2, 3, 18, 21, 22};
        for (unsigned i = 0; i < sizeof(ignored) / sizeof(*ignored); ++i)
            disposition(ignored[i], 1);
        disposition(1, (uintptr_t)hangup);
        disposition(15, (uintptr_t)hangup);
    }
}

static unsigned status_code(unsigned status)
{
    return !(status & 255) ? (status >> 8) & 255 : 128 + (status & 127);
}

static void record_status(struct shell_job *job, int pid, unsigned status)
{
    for (unsigned i = 0; i < job->count; ++i)
        if (job->pids[i] == pid)
        {
            unsigned bit = 1u << i;
            if (status == 0xffff)
                job->stopped &= ~bit;
            else if ((status & 255) == 0x7f)
            {
                job->stopped |= bit;
                job->result = 128 + ((status >> 8) & 255);
            }
            else
            {
                job->live &= ~bit;
                job->stopped &= ~bit;
                if (i + 1 == job->count)
                    job->result = status_code(status);
            }
        }
}

static int collect(struct shell_job *job, unsigned flags)
{
    int64_t result = lume_native_call(LUME_SYS_WAIT, -(int64_t)job->group, flags, 1);
    if (result >= 0)
        record_status(job, (int)((uint64_t)result >> 32), (unsigned)result);
    return result < 0 ? (int)result : 1;
}

void shell_jobs_poll(void)
{
    for (unsigned i = 0; i < JOB_LIMIT; ++i)
    {
        struct shell_job *job = jobs + i;
        if (!job->id || job == current)
            continue;
        while (collect(job, LUME_WAIT_STOPPED | LUME_WAIT_CONTINUED) > 0)
        {
        }
        if (!job->live)
        {
            printf("[%u] Done %s\n", job->id, job->command);
            memset(job, 0, sizeof(*job));
        }
    }
}

int shell_jobs_begin(const char *command, int background)
{
    shell_jobs_poll();
    current = NULL;
    if (!monitor && !background)
        return 0;
    current = &pending;
    *current = (struct shell_job){.background = background};
    snprintf(current->command, sizeof(current->command), "%s", command);
    if (monitor)
        (void)terminal(LUME_TTY_GET, 0, &shell_mode);
    return 0;
}

int shell_jobs_active(void)
{
    return current != NULL;
}

int shell_jobs_spawn(const char *path, unsigned argc, const char *const *argv, unsigned envc,
                     const char *const *envp)
{
    if (!current || current->count == JOB_PROCESSES)
        return -7;
    if (current == &pending)
    {
        struct shell_job *slot = NULL;
        for (unsigned i = 0; i < JOB_LIMIT; ++i)
            if (!jobs[i].id)
            {
                slot = jobs + i;
                break;
            }
        if (!slot)
            return -11;
        *slot = pending;
        slot->id = next_id++;
        current = slot;
    }
    fflush(stdout);
    fflush(stderr);
    int child = (int)lume_native_call(LUME_SYS_FORK, 0, 0, 0);
    if (!child)
    {
        disposition(1, 0);
        disposition(15, 0);
        struct lume_signal_request group = {.group = (int32_t)current->group};
        if (signal_request(LUME_SIGNAL_SET_GROUP, &group) < 0)
            goto failed;
        if (monitor && !current->background)
        {
            unsigned id = current->group ? current->group
                                         : (unsigned)lume_native_call(LUME_SYS_GETPID, 0, 0, 0);
            if (terminal(LUME_TTY_SET_GROUP, id, NULL))
                goto failed;
        }
        const unsigned reset[] = {2, 3, 18, 21, 22};
        for (unsigned i = 0; i < sizeof(reset) / sizeof(*reset); ++i)
            disposition(reset[i], 0);
        struct lume_signal_request mask = {.operation = 0, .flags = 1};
        (void)signal_request(LUME_SIGNAL_MASK, &mask);
        struct lume_run_request request = {.path = (uintptr_t)path,
                                           .argv = (uintptr_t)argv,
                                           .envp = (uintptr_t)envp,
                                           .argc = argc,
                                           .envc = envc};
        int error = (int)lume_native_call(LUME_SYS_EXEC, (uintptr_t)&request, sizeof(request), 0);
        fprintf(stderr, "%s: error %d\n", path, error);
    failed:
        lume_native_call(LUME_SYS_EXIT, 127, 0, 0);
        for (;;)
        {
        }
    }
    if (child < 0)
        return child;
    if (!current->group)
        current->group = (unsigned)child;
    struct lume_signal_request group = {.pid = child, .group = (int32_t)current->group};
    (void)signal_request(LUME_SIGNAL_SET_GROUP, &group);
    current->pids[current->count] = child;
    current->live |= 1u << current->count++;
    if (monitor && !current->background)
        (void)terminal(LUME_TTY_SET_GROUP, current->group, NULL);
    return child;
}

static int foreground(struct shell_job *job, int resume)
{
    if (monitor)
    {
        if (resume && job->mode_saved)
            (void)terminal(LUME_TTY_SET, 0, &job->mode);
        (void)terminal(LUME_TTY_SET_GROUP, job->group, NULL);
    }
    if (resume)
    {
        job->stopped = 0;
        (void)send_signal(job->group, 19);
    }
    while (job->live && job->stopped != job->live)
    {
        int result = collect(job, LUME_WAIT_BLOCK | LUME_WAIT_STOPPED);
        if (result == -4)
            continue;
        if (result < 0)
            break;
    }
    if (monitor)
    {
        job->mode_saved = !terminal(LUME_TTY_GET, 0, &job->mode);
        (void)terminal(LUME_TTY_SET_GROUP, shell_group, NULL);
        (void)terminal(LUME_TTY_SET, 0, &shell_mode);
    }
    unsigned result = job->result;
    if (job->live)
        printf("[%u] Stopped %s\n", job->id, job->command);
    else
        memset(job, 0, sizeof(*job));
    return (int)result;
}

int shell_jobs_complete(void)
{
    struct shell_job *job = current;
    current = NULL;
    if (!job || !job->count)
    {
        if (job)
            memset(job, 0, sizeof(*job));
        return -1;
    }
    if (job->background)
    {
        printf("[%u] %u\n", job->id, job->group);
        return 0;
    }
    return foreground(job, 0);
}

int shell_jobs_builtin(int argc, char **argv, unsigned *status)
{
    const char *command = argv[0];
    if (strcmp(command, "jobs") && strcmp(command, "fg") && strcmp(command, "bg"))
        return 0;
    shell_jobs_poll();
    *status = 0;
    if (!strcmp(command, "jobs"))
    {
        for (unsigned i = 0; i < JOB_LIMIT; ++i)
            if (jobs[i].id && jobs + i != current)
                printf("[%u] %s %s\n", jobs[i].id,
                       jobs[i].stopped == jobs[i].live ? "Stopped" : "Running", jobs[i].command);
        return 1;
    }
    unsigned id = 0;
    if (argc > 1)
    {
        const char *value = argv[1] + (argv[1][0] == '%');
        char *end;
        unsigned long number = strtoul(value, &end, 10);
        if (!number || *end || number > UINT32_MAX || argc != 2)
        {
            fprintf(stderr, "%s: invalid job\n", command);
            *status = 2;
            return 1;
        }
        id = (unsigned)number;
    }
    struct shell_job *job = NULL;
    for (unsigned i = 0; i < JOB_LIMIT; ++i)
        if (jobs[i].id && jobs + i != current &&
            (id ? jobs[i].id == id : !job || jobs[i].id > job->id))
            job = jobs + i;
    if (!job)
    {
        fprintf(stderr, "%s: no current job\n", command);
        *status = 1;
        return 1;
    }
    printf("%s\n", job->command);
    if (!strcmp(command, "fg"))
        *status = (unsigned)foreground(job, 1);
    else
    {
        job->stopped = 0;
        (void)send_signal(job->group, 19);
        job->background = 1;
    }
    return 1;
}
