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
        int result = assignment(state, (int)exported);
        state[bytes] = saved;
        if (result < 0)
            return result;
        state += bytes;
    }
    return 0;
}

static int shell_launch(struct shell_command *c)
{
    if (!strcmp(c->argv[0], "run") && c->argc > 1)
        return launch((int)c->argc - 1, c->argv + 1);
    if (!shell_builtin(c->argv[0]))
        return launch((int)c->argc, c->argv);
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
    return launch((int)c->argc + 3, argv);
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
    if (count == 1 && !commands[0].redirects)
        return commands[0].argc ? dispatch((int)commands[0].argc, commands[0].argv) : 0;
    int saved[3], pids[SHELL_STAGES], launched = 0, input = -1, stop = 0;
    int error = shell_save(saved);
    if (error < 0)
    {
        shell_error("shell: save descriptors", error);
        last_status = 1;
        return 0;
    }
    unsigned failed = 0, final_status = last_status;
    int final_pid = -1;
    for (int stage = 0; stage < count; ++stage)
    {
        unsigned setup = 1;
        int pair[2] = {-1, -1};
        if (stage + 1 < count && (error = app_fd_pair(pair, 0, LUME_FD_CLOEXEC)) < 0)
            goto stage_done;
        if (input >= 0 && (error = app_fd_dup(input, 0, 1, 0)) < 0)
            goto stage_done;
        if (pair[1] >= 0 && (error = app_fd_dup(pair[1], 1, 1, 0)) < 0)
            goto stage_done;
        setup = 0;
        struct shell_command *c = &commands[stage];
        for (unsigned i = 0; i < c->redirects; ++i)
            if ((error = shell_redirect(&redirects[c->first + i])) < 0)
                goto stage_done;
        if (count == 1)
        {
            if (c->argc)
                stop = dispatch((int)c->argc, c->argv);
            else
                last_status = 0;
        }
        else
        {
            last_status = 0;
            int pid = shell_launch(c);
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
        if (shell_restore(saved, 0) && count == 1 && !last_status)
            last_status = 1;
        if (input >= 0)
            (void)app_close(input);
        if (pair[1] >= 0)
            (void)app_close(pair[1]);
        input = pair[0];
        if (failed)
            break;
    }
    if (input >= 0)
        (void)app_close(input);
    shell_restore(saved, 1);
    for (int i = 0; i < launched; ++i)
        wait_for(pids[i]);
    if (count > 1 && final_pid < 0)
        last_status = final_status;
    if (failed)
        last_status = failed;
    return stop;
}
#endif
