#include "api.h"
#ifdef LUME_SHELL_FDS
#include "fd_client.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
extern char **environ;

#if defined(LUME_ARCH_AARCH64)
#define SHELL_VALUE_BYTES LUME_ENV_BYTES
#else
#define SHELL_VALUE_BYTES 129u
#endif
struct variable
{
    char name[32], value[SHELL_VALUE_BYTES];
    unsigned used, exported;
};
static struct variable variables[LUME_ENVS];
static unsigned last_status;
#if defined(LUME_ARCH_AARCH64)
#define SHELL_LINE_BYTES LUME_ARG_BYTES
#else
#define SHELL_LINE_BYTES 256u
#endif

static int list_segment(char *line, char **next)
{
    char quote = 0;
    *next = NULL;
    for (char *p = line; *p; ++p)
    {
        if (*p == '\\' && quote != '\'' && p[1])
        {
            ++p;
            continue;
        }
        if (quote)
        {
            if (*p == quote)
                quote = 0;
            continue;
        }
        if (*p == '\'' || *p == '"')
        {
            quote = *p;
            continue;
        }
        unsigned width = *p == ';' ? 1 : ((*p == '&' || *p == '|') && p[1] == *p ? 2 : 0);
        if (!width)
            continue;
        char *start = line;
        while (*start == ' ' || *start == '\t')
            ++start;
        if (start == p)
            return -22;
        int condition = width == 1 ? 1 : *p == '&' ? 2 : 3;
        *next = p + width;
        while (**next == ' ' || **next == '\t')
            ++*next;
        if (width == 2 && !**next)
            return -22;
        *p = 0;
        return condition;
    }
    return 0;
}

static int name_start(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static int name_char(char c)
{
    return name_start(c) || (c >= '0' && c <= '9');
}

static int name_valid(const char *s)
{
    if (!name_start(*s) || strlen(s) > 31)
        return 0;
    while (*++s)
        if (!name_char(*s))
            return 0;
    return 1;
}

static struct variable *find(const char *name)
{
    for (unsigned i = 0; i < LUME_ENVS; i++)
        if (variables[i].used && !strcmp(variables[i].name, name))
            return &variables[i];
    return NULL;
}

static int assign(const char *name, const char *value, int exported)
{
    if (!name_valid(name) || strlen(value) >= SHELL_VALUE_BYTES)
        return -22;
    struct variable *v = find(name);
    if (!v)
    {
        for (unsigned i = 0; i < LUME_ENVS; i++)
            if (!variables[i].used)
            {
                v = &variables[i];
                memset(v, 0, sizeof *v);
                break;
            }
    }
    if (!v)
        return -7;
    if (v->name != name)
        strcpy(v->name, name);
    if (v->value != value)
        strcpy(v->value, value);
    v->used = 1;
    if (exported >= 0)
        v->exported = (unsigned)exported;
    return 0;
}

static const char *value(const char *name)
{
    struct variable *v = find(name);
    return v ? v->value : "";
}

static int assignment(char *text, int exported)
{
    char *eq = strchr(text, '=');
    if (!eq)
        return -22;
    *eq = 0;
    int r = assign(text, eq + 1, exported);
    *eq = '=';
    return r;
}

static int environment(char data[LUME_ENV_BYTES], const char **out)
{
    unsigned used = 0, count = 0;
    for (unsigned i = 0; i < LUME_ENVS; i++)
        if (variables[i].used && variables[i].exported)
        {
            unsigned n = strlen(variables[i].name), m = strlen(variables[i].value);
            if (n + m + 2 > LUME_ENV_BYTES - used)
                return -7;
            out[count++] = data + used;
            memcpy(data + used, variables[i].name, n);
            used += n;
            data[used++] = '=';
            memcpy(data + used, variables[i].value, m + 1);
            used += m + 1;
        }
    out[count] = NULL;
    return (int)count;
}

static uint32_t pointer(const void *p)
{
    return (uint32_t)(uintptr_t)p;
}

static unsigned number(const char *s)
{
    unsigned n = 0;
    while (*s >= '0' && *s <= '9')
    {
        n = n * 10 + (unsigned)(*s - '0');
        s++;
    }
    return n;
}

static void ps(void)
{
    app_puts("PID PPID UID STATE RUNS CORES NAME\n");
    for (unsigned i = 0; i < 8; i++)
    {
        struct lume_process_info p = {0};
        if (lume_syscall(LUME_PS, i, pointer(&p), 0) <= 0)
            continue;
        app_number(p.pid);
        app_puts(" ");
        app_number(p.parent);
        app_puts(" ");
        app_number(p.uid);
        app_puts(" ");
        app_number(p.state);
        app_puts(" ");
        app_number(p.runs);
        app_puts(" ");
        app_number(p.cores);
        app_puts(" ");
        app_puts(p.name);
        app_puts("\n");
    }
}

/* Bounded shell grammar: quotes, escapes and scalar expansion. No word
 * splitting, globbing, command substitution, pipelines or job control yet. */
static int split(const char *line, char output[LUME_ARG_BYTES], char **argv)
{
    const char *read = line;
    unsigned used = 0, argc = 0;
#define EMIT(c)                                                                                    \
    do                                                                                             \
    {                                                                                              \
        if (used >= LUME_ARG_BYTES - 1)                                                            \
            return -7;                                                                             \
        output[used++] = (c);                                                                      \
    } while (0)
    while (*read)
    {
        while (*read == ' ' || *read == '\t')
            read++;
        if (!*read)
            break;
        if (argc == LUME_ARGS)
            return -7;
        argv[argc++] = output + used;
        char quote = 0;
        while (*read)
        {
            char c = *read++;
            if (c == '\\' && quote != '\'')
            {
                if (!*read)
                    return -22;
                EMIT(*read++);
                continue;
            }
            if (quote && c == quote)
            {
                quote = 0;
                continue;
            }
            if (!quote && (c == '\'' || c == '"'))
            {
                quote = c;
                continue;
            }
            if (!quote && (c == ' ' || c == '\t'))
                break;
            if (c == '$' && quote != '\'')
            {
                char name[32];
                unsigned n = 0;
                const char *s;
                if (*read == '?')
                {
                    read++;
                    char status[12];
                    snprintf(status, sizeof status, "%u", last_status);
                    for (s = status; *s; s++)
                        EMIT(*s);
                    continue;
                }
                int braces = *read == '{';
                if (braces)
                    read++;
                if (!name_start(*read))
                {
                    if (braces)
                        return -22;
                    EMIT('$');
                    continue;
                }
                while (name_char(*read))
                {
                    if (n >= 31)
                        return -7;
                    name[n++] = *read++;
                }
                name[n] = 0;
                if (braces)
                {
                    if (*read != '}')
                        return -22;
                    read++;
                }
                for (s = value(name); *s; s++)
                    EMIT(*s);
                continue;
            }
            EMIT(c);
        }
        if (quote)
            return -22;
        EMIT(0);
    }
    argv[argc] = NULL;
    return (int)argc;
#undef EMIT
}

static int executable(const char *path)
{
    struct lume_stat s;
    return app_stat(path, &s) == 0 && (s.mode & 0x8000);
}

static void execute(int argc, char **argv)
{
    char path[128];
    const char *command = argv[0];
    size_t n = strlen(command);
    if (n > 100)
    {
        app_puts("run: path too long\n");
        last_status = 126;
        return;
    }
    const char *ext = sizeof(LUME_APP_SUFFIX) > 1 && n >= sizeof(LUME_APP_SUFFIX) - 1 &&
                              !strcmp(command + n - (sizeof(LUME_APP_SUFFIX) - 1), LUME_APP_SUFFIX)
                          ? ""
                          : LUME_APP_SUFFIX;
    if (strchr(command, '/'))
    {
        snprintf(path, sizeof path, "%s%s", command, ext);
    }
    else
    {
        const char *p = value("PATH");
        int found = 0;
        do
        {
            const char *end = strchr(p, ':');
            size_t length = end ? (size_t)(end - p) : strlen(p);
            if (length + n + strlen(ext) + 2 < sizeof path)
            {
                if (length)
                {
                    memcpy(path, p, length);
                    path[length] = '/';
                    strcpy(path + length + 1, command);
                    strcat(path, ext);
                }
                else
                    snprintf(path, sizeof path, "%s%s", command, ext);
                if (executable(path))
                {
                    found = 1;
                    break;
                }
            }
            if (!end)
                break;
            p = end + 1;
        } while (1);
        if (!found)
        {
            app_puts(command);
            app_puts(": command not found\n");
            last_status = 127;
            return;
        }
    }
    char data[LUME_ENV_BYTES];
    const char *env[LUME_ENVS + 1];
    int count = environment(data, env);
    if (count < 0)
    {
        app_error("environment", count);
        last_status = 2;
        return;
    }
#ifdef LUME_SHELL_FDS
    int pid = app_spawn_fds(path, (unsigned)argc, (const char *const *)argv, (unsigned)count, env);
#else
    int pid = app_spawn_env(path, (unsigned)argc, (const char *const *)argv, (unsigned)count, env);
#endif
    if (pid < 0)
    {
        app_puts("run: load failed\n");
        app_error(command, pid);
        last_status = 126;
        return;
    }
    struct lume_wait_status status;
    int r = app_wait(pid, &status);
    if (r < 0)
    {
        app_error("wait", r);
        last_status = 1;
    }
    else
        last_status = status.code;
}

static void echo(int argc, char **argv)
{
    int fd = 1, last = argc;
    for (int i = 1; i < argc; i++)
        if (app_equal(argv[i], ">") || app_equal(argv[i], ">>"))
        {
            if (i + 2 != argc)
            {
                app_puts("echo: use > file or >> file\n");
                last_status = 2;
                return;
            }
            fd = app_open(argv[i + 1],
                          LUME_WRONLY | LUME_CREAT | (argv[i][1] ? LUME_APPEND : LUME_TRUNC));
            if (fd < 0)
            {
                app_error("echo", fd);
                last_status = 1;
                return;
            }
            last = i;
            break;
        }
    int error = 0;
    char text[256];
    unsigned used = 0;
    for (int i = 1; i < last; i++)
    {
        if (i > 1)
            text[used++] = ' ';
        for (const char *s = argv[i]; *s; s++)
        {
            if (used >= 254)
            {
                error = -7;
                break;
            }
            text[used++] = *s;
        }
        if (error)
            break;
    }
    if (!error)
    {
        text[used++] = '\n';
        int r = app_write(fd, text, used);
        if (r != (int)used)
            error = r < 0 ? r : -28;
    }
    if (fd != 1)
    {
        int r = app_close(fd);
        if (r < 0)
            error = r;
    }
    if (error)
    {
        app_error("echo", error);
        last_status = 1;
    }
}

static void profile(void)
{
    int fd = app_open("/etc/profile", LUME_RDONLY);
    if (fd < 0)
        return;
    char text[513];
    int bytes = app_read(fd, text, 512);
    app_close(fd);
    if (bytes < 0)
        return;
    text[bytes] = 0;
    char *line = text;
    while (*line)
    {
        char *end = strchr(line, '\n');
        if (end)
            *end = 0;
        if (*line && *line != '#')
        {
            char expanded[LUME_ARG_BYTES], *argv[LUME_ARGS + 1];
            int argc = split(line, expanded, argv);
            if (argc == 2 && app_equal(argv[0], "export"))
            {
                int r = assignment(argv[1], 1);
                if (r < 0)
                    app_error("profile", r);
            }
            else
                app_puts("profile: expected export NAME=value\n");
        }
        if (!end)
            break;
        line = end + 1;
    }
}

int app_main(unsigned initial_argc, char **initial_argv)
{
    unsigned command_mode = initial_argc == 3 && !strcmp(initial_argv[1], "-c"), executed = 0;
    int script = !command_mode && initial_argc == 2 ? app_open(initial_argv[1], LUME_RDONLY) : -1;
    unsigned char script_buffer[512];
    unsigned script_at = 0, script_bytes = 0, script_eof = 0;
    if (!command_mode && initial_argc == 2 && script < 0)
        return 2;
    for (char **s = environ; *s; s++)
        if (assignment(*s, 1) < 0)
            app_puts("shell: initial environment exceeds limits\n");
    if (!command_mode && script < 0)
    {
        profile();
        (void)lume_syscall(LUME_MKDIR, pointer("/root"), 0, 0);
        (void)lume_syscall(LUME_CHDIR, pointer(value("HOME")), 0, 0);
    }
    if (!command_mode && script < 0)
#ifdef LUME_SHELL_FDS
        app_puts("LumeOS SSH shell / root / syscall ABI0\n");
#else
        app_puts("LumeOS shell / root / syscall ABI0\nType help for commands.\n");
#endif
    char pending[SHELL_LINE_BYTES] = {0};
    unsigned condition = 0;
    for (;;)
    {
        if (command_mode && executed && !pending[0])
            return (int)last_status;
        char line[SHELL_LINE_BYTES];
        if (pending[0])
        {
            strcpy(line, pending);
            pending[0] = 0;
            goto line_ready;
        }
        char cwd[128];
        if (lume_syscall(LUME_GETCWD, pointer(cwd), sizeof cwd, 0) < 0)
        {
            cwd[0] = '?';
            cwd[1] = 0;
        }
        (void)assign("PWD", cwd, 1);
#ifdef LUME_SHELL_FDS
        int geometry = 0;
#else
        int geometry = lume_syscall(LUME_TERMINAL, 0, 0, 0);
#endif
        if (!command_mode && script < 0 && geometry > 0)
        {
            char size[12];
            snprintf(size, sizeof size, "%u", (unsigned)geometry & 65535);
            (void)assign("COLUMNS", size, 1);
            snprintf(size, sizeof size, "%u", (unsigned)geometry >> 16);
            (void)assign("LINES", size, 1);
        }
        if (!command_mode && script < 0)
        {
            app_puts("root@lume:");
            app_puts(cwd);
            app_puts("# ");
        }
        unsigned used = 0, overflow = 0, escape = 0;
        if (command_mode)
        {
            if (strlen(initial_argv[2]) >= sizeof line)
                return 2;
            strcpy(line, initial_argv[2]);
            executed = 1;
            goto line_ready;
        }
        for (;;)
        {
            int32_t ch;
            if (script >= 0)
            {
                if (script_at == script_bytes && !script_eof)
                {
                    int n = app_read(script, script_buffer, sizeof script_buffer);
                    if (n < 0)
                    {
                        app_error("shell: read", n);
                        return 2;
                    }
                    script_at = 0;
                    script_bytes = (unsigned)n;
                    script_eof = n == 0;
                }
                if (script_at == script_bytes && !used)
                    return (int)last_status;
                ch = script_at < script_bytes ? script_buffer[script_at++] : '\n';
            }
            else
            {
#ifdef LUME_SHELL_FDS
                unsigned char byte;
                int n = app_read(0, &byte, 1);
                if (!n)
                    return 0;
                ch = n < 0 ? n : byte;
#else
                ch = lume_syscall(LUME_GETCHAR, 0, 0, 0);
#endif
            }
            if (ch < 0)
            {
                int wait = lume_syscall(LUME_TERMINAL, 5, 1000, 0);
                if (wait == -22 || wait == -38 || wait == -95)
                    lume_syscall(LUME_YIELD, 0, 0, 0);
                continue;
            }
            if (ch == 27)
            {
                escape = 1;
                continue;
            }
            if (escape)
            {
                if (escape == 1)
                {
                    escape = (ch == '[' || ch == 'O') ? 2 : 0;
                    continue;
                }
                if (ch >= 0x40 && ch <= 0x7e)
                    escape = 0;
                continue;
            }
            if (ch == '\r' || ch == '\n')
            {
                if (script < 0)
                    app_puts("\n");
                break;
            }
            if (ch == 8 || ch == 127)
            {
                if (used)
                {
                    used--;
                    while (used && ((unsigned char)line[used] & 0xc0) == 0x80)
                        used--;
                    app_puts("\b \b");
                }
                continue;
            }
            if (ch == 3)
            {
                used = 0;
                app_puts("^C\n");
                break;
            }
            if (ch >= 32 && ch != 127 && ch < 256 && used + 1 < sizeof line)
            {
                line[used++] = (char)ch;
                char e[2] = {(char)ch, 0};
                if (script < 0)
                    app_puts(e);
            }
            else if (ch >= 32 && ch != 127 && ch < 256)
                overflow = 1;
        }
        if (overflow)
        {
            app_error("input", -7);
            last_status = 2;
            continue;
        }
        line[used] = 0;
    line_ready:;
        unsigned gate = condition;
        char *next;
        int separator = list_segment(line, &next);
        if (separator < 0)
        {
            app_error("shell: command list", separator);
            last_status = 2;
            condition = 0;
            continue;
        }
        condition = (unsigned)separator;
        if (next)
            strcpy(pending, next);
        if ((gate == 2 && last_status) || (gate == 3 && !last_status))
            continue;
        char expanded[LUME_ARG_BYTES], *argv[LUME_ARGS + 1];
        int argc = split(line, expanded, argv);
        if (argc < 0)
        {
            app_error("arguments", argc);
            last_status = 2;
            continue;
        }
        if (!argc)
            continue;
        char *cmd = argv[0];
        const char *arg = argc > 1 ? argv[1] : "";
        if (argc == 1 && strchr(cmd, '='))
        {
            int r = assignment(cmd, -1);
            if (r < 0)
                app_error("assignment", r);
            last_status = r < 0 ? 2 : 0;
            continue;
        }
        unsigned previous = last_status;
        last_status = 0;
        if (app_equal(cmd, "export"))
        {
            if (argc == 1)
            {
                for (unsigned i = 0; i < LUME_ENVS; i++)
                    if (variables[i].used && variables[i].exported)
                        printf("%s=%s\n", variables[i].name, variables[i].value);
            }
            for (int i = 1; i < argc; i++)
            {
                int r;
                if (strchr(argv[i], '='))
                    r = assignment(argv[i], 1);
                else
                    r = assign(argv[i], value(argv[i]), 1);
                if (r < 0)
                {
                    app_error("export", r);
                    last_status = 2;
                }
            }
        }
        else if (app_equal(cmd, "unset"))
        {
            for (int i = 1; i < argc; i++)
            {
                if (!name_valid(argv[i]))
                {
                    app_error("unset", -22);
                    last_status = 2;
                    continue;
                }
                struct variable *v = find(argv[i]);
                if (v)
                    v->used = 0;
            }
        }
        else if (app_equal(cmd, "set"))
        {
            for (unsigned i = 0; i < LUME_ENVS; i++)
                if (variables[i].used)
                    printf("%s=%s%s\n", variables[i].name, variables[i].value,
                           variables[i].exported ? " [exported]" : "");
        }
        else if (app_equal(cmd, "help"))
            app_puts(
                "Builtins: help uname id ps uptime cd echo run export unset set sched protecttest rm mv exit\nPATH programs: busybox ls dir less cat head wc printenv basename dirname pwd true false\nMore programs: sort cut cmp cksum crc32 expand unexpand mkdir rmdir\nEditor: vi FILE; video [800x600|1024x768|font FILE]\n$NAME ${NAME} $?; quotes; FAT32/8.3; echo text > file or >> file.\nDiagnostics: run /usr/libexec/fstest" LUME_APP_SUFFIX
                "\n");
        else if (app_equal(cmd, "uname"))
#if defined(LUME_ARCH_AARCH64)
            app_puts("LumeOS / Lume Kernel / AArch64 / Raspberry Pi 5\n");
#elif defined(LUME_ARCH_R32)
            app_puts("LumeOS / Lume Kernel / R32 ISA v1 / single core / fair scheduler\n");
#else
            app_puts("LumeOS / Lume Kernel / X32 ISA v1 / SMP2 / round-robin\n");
#endif
        else if (app_equal(cmd, "id"))
            app_puts("uid=0(root) gid=0(root)\n");
        else if (app_equal(cmd, "ps"))
            ps();
        else if (app_equal(cmd, "uptime"))
        {
            app_number((uint32_t)lume_syscall(LUME_INFO, LUME_UPTIME, 0, 0));
            app_puts(" ms\n");
        }
        else if (app_equal(cmd, "echo"))
            echo(argc, argv);
        else if (app_equal(cmd, "cd"))
        {
            int r = lume_syscall(LUME_CHDIR, pointer(*arg ? arg : value("HOME")), 0, 0);
            if (r < 0)
            {
                app_error("cd", r);
                last_status = 1;
            }
        }
        else if (app_equal(cmd, "rm"))
        {
            if (argc != 2)
            {
                app_puts("usage: rm file\n");
                last_status = 2;
                continue;
            }
            int r = lume_syscall(LUME_UNLINK, pointer(arg), 0, 0);
            if (r < 0)
            {
                app_error(cmd, r);
                last_status = 1;
            }
        }
        else if (app_equal(cmd, "mv"))
        {
            if (argc != 3)
            {
                app_puts("usage: mv source destination\n");
                last_status = 2;
                continue;
            }
            int r = lume_syscall(LUME_RENAME, pointer(arg), pointer(argv[2]), 0);
            if (r < 0)
            {
                app_error("mv", r);
                last_status = 1;
            }
        }
        else if (app_equal(cmd, "run"))
        {
            if (argc < 2)
                app_puts("usage: run path [args]\n");
            else
                execute(argc - 1, argv + 1);
        }
        else if (app_equal(cmd, "protecttest"))
        {
            char *args[] = {"/usr/libexec/probe" LUME_APP_SUFFIX ""};
            execute(1, args);
        }
        else if (app_equal(cmd, "sched"))
        {
            if (*arg && lume_syscall(LUME_QUANTUM, number(arg), 0, 0) < 0)
                app_puts("sched: quantum must be100..1000 ms\n");
            app_puts("round_robin quantum_ms=");
            app_number((uint32_t)lume_syscall(LUME_INFO, LUME_QUANTUM_MS, 0, 0));
            app_puts(" cores=2\n");
        }
#ifdef LUME_ARCH_AARCH64
        else if (app_equal(cmd, "reboot"))
            return 0;
#endif
        else if (app_equal(cmd, "exit"))
            return argc > 1 ? (int)number(arg) : (int)previous;
        else
            execute(argc, argv);
    }
}
