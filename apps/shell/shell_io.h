#ifndef LUME_SHELL_IO_H
#define LUME_SHELL_IO_H

#define SHELL_STAGES 6u

struct shell_redirect
{
    unsigned fd, type;
    char *path;
};

struct shell_command
{
    char **argv;
    unsigned argc, first, redirects;
};

struct shell_stage
{
    int input, pipe[2];
    const struct shell_redirect *redirects;
    unsigned count;
};

static int shell_parse(const char *line, char *expanded, char **arguments,
                       struct shell_command *commands, struct shell_redirect *redirects)
{
    char *tokens[LUME_ARGS + 1];
    unsigned char types[LUME_ARGS], fds[LUME_ARGS];
    int n = tokenize(line, expanded, tokens, types, fds);
    if (n < 0)
        return n;
    unsigned stage = 0, used = 0, redirected = 0;
    commands[0] = (struct shell_command){.argv = arguments};
    for (int i = 0; i < n; ++i)
    {
        struct shell_command *c = &commands[stage];
        if (types[i] == '|')
        {
            if (!c->argc || i + 1 == n || stage + 1 == SHELL_STAGES)
                return -22;
            arguments[used++] = NULL;
            commands[++stage] =
                (struct shell_command){.argv = arguments + used, .first = redirected};
        }
        else if (types[i])
        {
            if (fds[i] > 2 || i + 1 == n || types[i + 1])
                return -22;
            redirects[redirected++] = (struct shell_redirect){fds[i], types[i], tokens[i + 1]};
            ++c->redirects;
            ++i;
        }
        else
        {
            arguments[used++] = tokens[i];
            ++c->argc;
        }
    }
    arguments[used] = NULL;
    return n ? (int)stage + 1 : 0;
}

static int shell_redirect(const struct shell_redirect *r)
{
    if (r->type == 'd' || r->type == 'i')
    {
        if (!strcmp(r->path, "-"))
        {
            int error = app_close((int)r->fd);
            return error == -9 ? 0 : error;
        }
        if (r->path[0] < '0' || r->path[0] > '2' || r->path[1])
            return -22;
        return app_fd_dup(r->path[0] - '0', (int)r->fd, 1, 0);
    }
    unsigned flags = r->type == '<'
                         ? LUME_RDONLY
                         : LUME_WRONLY | LUME_CREAT | (r->type == 'a' ? LUME_APPEND : LUME_TRUNC);
    int fd = app_open(r->path, flags);
    if (fd < 0)
        return fd;
    int result = app_fd_dup(fd, (int)r->fd, 1, 0);
    if (fd != (int)r->fd)
        (void)app_close(fd);
    return result;
}

static int shell_redirections(const struct shell_stage *stage)
{
    for (unsigned i = 0; i < stage->count; ++i)
    {
        int error = shell_redirect(&stage->redirects[i]);
        if (error < 0)
            return error;
    }
    return 0;
}

static int shell_connect_stage(const struct shell_stage *stage)
{
    int error = stage->input < 0 ? 0 : app_fd_dup(stage->input, 0, 1, 0);
    if (error >= 0 && stage->pipe[1] >= 0)
        error = app_fd_dup(stage->pipe[1], 1, 1, 0);
    return error;
}

#ifdef LUME_SHELL_JOB_CONTROL
int shell_job_prepare(const struct shell_stage *stage)
{
    int error = shell_connect_stage(stage);
    /* Close the temporary pipeline descriptors before opening redirection paths. */
    const int inherited[] = {stage->input, stage->pipe[0], stage->pipe[1]};
    for (unsigned i = 0; i < sizeof(inherited) / sizeof(inherited[0]); ++i)
        if (inherited[i] >= 0)
            (void)app_close(inherited[i]);
    if (error >= 0)
        error = shell_redirections(stage);
    if (error < 0)
        shell_error("shell: redirection", error);
    return error;
}
#endif

static int shell_pipe(int ends[2])
{
    int result = app_fd_pair(ends, 0, LUME_FD_CLOEXEC);
    if (result < 0)
        return result;
    for (unsigned i = 0; i < 2; ++i)
        if (ends[i] < 3)
        {
            int fd = app_fd_dup(ends[i], 3, 0, LUME_FD_CLOEXEC);
            if (fd < 0)
            {
                (void)app_close(ends[0]);
                (void)app_close(ends[1]);
                ends[0] = ends[1] = -1;
                return fd;
            }
            (void)app_close(ends[i]);
            ends[i] = fd;
        }
    return 0;
}

static int shell_save(int saved[3])
{
    (void)fflush(stdout);
    (void)fflush(stderr);
    for (int i = 0; i < 3; ++i)
    {
        saved[i] = app_fd_dup(i, 3, 0, LUME_FD_CLOEXEC);
        if (saved[i] < 0 && saved[i] != -9)
        {
            for (int j = 0; j < i; ++j)
                if (saved[j] >= 0)
                    (void)app_close(saved[j]);
            return saved[i];
        }
    }
    return 0;
}

static int shell_restore(const int saved[3], unsigned close_saved)
{
    int failed = fflush(stdout) < 0;
    failed |= fflush(stderr) < 0;
    lume_stdio_reset();
    for (int i = 0; i < 3; ++i)
    {
        if (saved[i] >= 0)
        {
            (void)app_fd_dup(saved[i], i, 1, 0);
            if (close_saved)
                (void)app_close(saved[i]);
        }
        else
            (void)app_close(i);
    }
    clearerr(stdin);
    clearerr(stdout);
    clearerr(stderr);
    return failed;
}

static int shell_builtin(const char *name)
{
#if defined(LUME_ARCH_AARCH64) && !defined(LUME_SHELL_FDS)
    if (!strcmp(name, "boot"))
        return 1;
#endif
#ifdef LUME_SHELL_JOB_CONTROL
    if (!strcmp(name, "jobs") || !strcmp(name, "fg") || !strcmp(name, "bg"))
        return 1;
#endif
    static const char *const names[] = {"export",      "unset",  "set",    "help", "uname", "id",
                                        "ps",          "uptime", "echo",   "cd",   "rm",    "mv",
                                        "protecttest", "sched",  "reboot", "exit", "run"};
    for (unsigned i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
        if (!strcmp(name, names[i]))
            return 1;
    return strchr(name, '=') != NULL;
}

static int shell_import(char *state)
{
    while (*state)
    {
        if (*state != '0' && *state != '1')
            return -22;
        unsigned exported = (unsigned)(*state++ - '0'), bytes = 0;
        if (*state < '0' || *state > '9')
            return -22;
        while (*state >= '0' && *state <= '9')
        {
            unsigned digit = (unsigned)(*state++ - '0');
            if (bytes > (LUME_ARG_BYTES - digit) / 10)
                return -7;
            bytes = bytes * 10 + digit;
        }
        if (*state++ != ':' || bytes > strlen(state))
            return -22;
        char saved = state[bytes];
        state[bytes] = 0;
        int result = assignment(state, (int)exported, 0);
        state[bytes] = saved;
        if (result < 0)
            return result;
        state += bytes;
    }
    return 0;
}

static int shell_launch(struct shell_command *c, const struct shell_stage *stage)
{
    if (c->argc > 1 && !strcmp(c->argv[0], "run"))
        return launch_prepared((int)c->argc - 1, c->argv + 1, stage);
    if (c->argc && !shell_builtin(c->argv[0]))
        return launch_prepared((int)c->argc, c->argv, stage);
    if (c->argc + 3 > LUME_ARGS)
    {
        last_status = 2;
        shell_error("shell: arguments", -7);
        return -1;
    }
    char *argv[LUME_ARGS + 1];
    char state[LUME_ARG_BYTES];
    unsigned used = 0;
    state[0] = 0;
    for (unsigned i = 0; i < LUME_ENVS; ++i)
        if (variables[i].used)
        {
            unsigned bytes = (unsigned)(strlen(variables[i].name) + strlen(variables[i].value) + 1);
            int n = snprintf(state + used, sizeof(state) - used, "%u%u:%s=%s",
                             variables[i].exported, bytes, variables[i].name, variables[i].value);
            if (n < 0 || (unsigned)n >= sizeof(state) - used)
            {
                shell_error("shell: variable snapshot", -7);
                last_status = 2;
                return -1;
            }
            used += (unsigned)n;
        }
    argv[0] = "/bin/sh" LUME_APP_SUFFIX;
    argv[1] = "--builtin";
    argv[2] = state;
    for (unsigned i = 0; i < c->argc; ++i)
        argv[i + 3] = c->argv[i];
    argv[c->argc + 3] = NULL;
    return launch_prepared((int)c->argc + 3, argv, stage);
}

static int shell_line(const char *line)
{
    char expanded[LUME_ARG_BYTES], *arguments[LUME_ARGS + SHELL_STAGES];
    struct shell_command commands[SHELL_STAGES];
    struct shell_redirect redirects[LUME_ARGS];
    int count = shell_parse(line, expanded, arguments, commands, redirects);
    if (count < 0)
    {
        shell_error("shell: syntax", count);
        last_status = 2;
        return 0;
    }
    if (!count)
        return 0;
    int background = 0;
    bool prepare_in_child = false;
#ifdef LUME_SHELL_JOB_CONTROL
    background = background_command;
    const struct shell_command *first = &commands[0];
    prepare_in_child =
        background || count > 1 ||
        (first->redirects && first->argc &&
         (!shell_builtin(first->argv[0]) || (first->argc > 1 && !strcmp(first->argv[0], "run"))));
    int job_error = shell_jobs_begin(line, background, prepare_in_child);
    if (job_error < 0)
    {
        shell_error("shell: jobs", job_error);
        last_status = 1;
        return 0;
    }
#endif
    if (count == 1 && !commands[0].redirects && !background)
    {
        int stop = commands[0].argc ? dispatch((int)commands[0].argc, commands[0].argv) : 0;
#ifdef LUME_SHELL_JOB_CONTROL
        int status = shell_jobs_complete();
        if (status >= 0)
            last_status = (unsigned)status;
#endif
        return stop;
    }
    int saved[3], pids[SHELL_STAGES], launched = 0, input = -1, stop = 0;
    int error = prepare_in_child ? 0 : shell_save(saved);
    if (error < 0)
    {
        shell_error("shell: save descriptors", error);
        last_status = 1;
#ifdef LUME_SHELL_JOB_CONTROL
        (void)shell_jobs_complete();
#endif
        return 0;
    }
    unsigned failed = 0, final_status = last_status;
    int final_pid = -1;
    for (int stage = 0; stage < count; ++stage)
    {
        unsigned setup = 1;
        struct shell_command *c = &commands[stage];
        struct shell_stage io = {
            .input = input,
            .pipe = {-1, -1},
            .redirects = &redirects[c->first],
            .count = c->redirects,
        };
        error = 0;
        if (stage + 1 < count && (error = shell_pipe(io.pipe)) < 0)
            goto stage_done;
        if (!prepare_in_child)
        {
            if ((error = shell_connect_stage(&io)) < 0)
                goto stage_done;
            setup = 0;
            if ((error = shell_redirections(&io)) < 0)
                goto stage_done;
        }
        if (count == 1 && !prepare_in_child)
        {
            if (c->argc)
                stop = dispatch((int)c->argc, c->argv);
            else
                last_status = 0;
        }
        else
        {
            last_status = 0;
            int pid = shell_launch(c, prepare_in_child ? &io : NULL);
            if (pid < 0)
            {
                if (stage + 1 == count)
                    final_status = last_status;
                error = 0;
                goto stage_done;
            }
            pids[launched++] = pid;
            if (stage + 1 == count)
                final_pid = pid;
        }
    stage_done:
        if (error < 0)
        {
            shell_error("shell: redirection", error);
            if (setup || count == 1)
                failed = 1;
            if (stage + 1 == count)
                final_status = 1;
        }
        if (!prepare_in_child && shell_restore(saved, 0) && count == 1 && !last_status)
            last_status = 1;
        if (input >= 0)
            (void)app_close(input);
        if (io.pipe[1] >= 0)
            (void)app_close(io.pipe[1]);
        input = io.pipe[0];
        if (failed)
            break;
    }
    if (input >= 0)
        (void)app_close(input);
    if (!prepare_in_child)
        shell_restore(saved, 1);
    for (int i = 0; i < launched; ++i)
        wait_for(pids[i]);
#ifdef LUME_SHELL_JOB_CONTROL
    int status = shell_jobs_complete();
    if (status >= 0)
        last_status = (unsigned)status;
#endif
    if (count > 1 && final_pid < 0)
        last_status = final_status;
    if (failed)
        last_status = failed;
    return stop;
}
#endif
