#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define environ shell_environment
#define app_main shell_main
char **shell_environment;
#include "../../apps/shell/shell.c"
#undef app_main
#undef environ

static char output[16384];
static char errors[16384];
static unsigned error_bytes;
static unsigned output_bytes, input_bytes, input_at, read_calls, read_limit, failures;
static unsigned read_error, wait_calls, checks;
static const char *input;

#define CHECK(condition)                                                                           \
    do                                                                                             \
    {                                                                                              \
        ++checks;                                                                                  \
        if (!(condition))                                                                          \
        {                                                                                          \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition);                        \
            fprintf(stderr, "output=[%s] reads=%u input=%u/%u\n", output, read_calls, input_at,    \
                    input_bytes);                                                                  \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

void app_puts(const char *text)
{
    size_t bytes = strlen(text);
    CHECK(bytes < sizeof output - output_bytes);
    memcpy(output + output_bytes, text, bytes + 1);
    output_bytes += (unsigned)bytes;
}

void app_number(uint32_t value)
{
    char text[16];
    snprintf(text, sizeof text, "%u", value);
    app_puts(text);
}

unsigned app_length(const char *text)
{
    return (unsigned)strlen(text);
}

int app_equal(const char *a, const char *b)
{
    return !strcmp(a, b);
}

_Noreturn void app_exit(unsigned code)
{
    (void)code;
    abort();
}

int app_open(const char *name, unsigned flags)
{
    (void)flags;
    return !strcmp(name, "/script") ? 17 : -2;
}

int app_close(int fd)
{
    (void)fd;
    return 0;
}

int app_read(int fd, void *buffer, unsigned bytes)
{
    CHECK(fd == 17);
    CHECK(bytes == 512);
    ++read_calls;
    if (read_error)
        return -5;
    unsigned count = input_bytes - input_at;
    if (count > bytes)
        count = bytes;
    if (read_limit && count > read_limit)
        count = read_limit;
    memcpy(buffer, input + input_at, count);
    input_at += count;
    return (int)count;
}

int app_write(int fd, const void *data, unsigned bytes)
{
    CHECK(fd == 1 || fd == 2);
    if (fd == 2)
    {
        ++failures;
        CHECK(bytes < sizeof errors - error_bytes);
        memcpy(errors + error_bytes, data, bytes);
        error_bytes += bytes;
        errors[error_bytes] = 0;
        return (int)bytes;
    }
    CHECK(bytes < sizeof output - output_bytes);
    memcpy(output + output_bytes, data, bytes);
    output_bytes += bytes;
    output[output_bytes] = 0;
    return (int)bytes;
}

int app_seek(int fd, int32_t offset, unsigned mode)
{
    (void)fd;
    (void)offset;
    (void)mode;
    return -38;
}

int app_sync(int fd)
{
    (void)fd;
    return 0;
}

int app_stat(const char *name, struct lume_stat *status)
{
    (void)name;
    (void)status;
    return -2;
}

void lume_stdio_raw(int enabled)
{
    (void)enabled;
}

void lume_stdio_reset(void)
{
}

int app_spawn(const char *name, unsigned argc, const char *const *argv)
{
    (void)name;
    (void)argc;
    (void)argv;
    return -2;
}

int app_spawn_env(const char *name, unsigned argc, const char *const *argv, unsigned envc,
                  const char *const *envp)
{
    (void)envc;
    (void)envp;
    return app_spawn(name, argc, argv);
}

int app_wait(int pid, struct lume_wait_status *status)
{
    (void)pid;
    (void)status;
    return -10;
}

void app_error(const char *name, int error)
{
    (void)name;
    (void)error;
    ++failures;
}

int32_t lume_syscall(uint32_t number, uint32_t a, uint32_t b, uint32_t c)
{
    (void)a;
    (void)b;
    (void)c;
    if (number == LUME_GETCWD)
        return -38;
    if ((number == LUME_TERMINAL && a == 5) || number == LUME_YIELD || number == LUME_SLEEP)
        ++wait_calls;
    return 0;
}

static void prepare(const char *text, unsigned short_read, unsigned error)
{
    static char home[] = "HOME=/root";
    static char path[] = "PATH=/bin:/usr/bin";
    static char *environment[] = {home, path, NULL};
    shell_environment = environment;
    memset(variables, 0, sizeof variables);
    last_status = 0;
    input = text;
    input_bytes = (unsigned)strlen(text);
    input_at = output_bytes = read_calls = failures = wait_calls = 0;
    output[0] = 0;
    errors[0] = 0;
    error_bytes = 0;
    read_limit = short_read;
    read_error = error;
}

static int script(const char *text, unsigned short_read, unsigned error)
{
    char name[] = "shell", filename[] = "/script";
    char *argv[] = {name, filename, NULL};
    prepare(text, short_read, error);
    return shell_main(2, argv);
}

int main(void)
{
    CHECK(script("", 0, 0) == 0);
    CHECK(read_calls == 1 && !output_bytes);
    CHECK(script("echo done", 0, 0) == 0);
    CHECK(!strcmp(output, "done\n") && read_calls == 2);
    CHECK(script("echo first\necho second\n", 0, 0) == 0);
    CHECK(!strcmp(output, "first\nsecond\n") && read_calls == 2);
    CHECK(script("echo 'short reads'\nexit 17\n", 3, 0) == 17);
    CHECK(!strcmp(output, "short reads\n"));
    CHECK(script("echo ignored\n", 0, 1) == 2);
    CHECK(failures == 1 && read_calls == 1 && wait_calls == 0);

    char source[4096] = "", expected[4096] = "";
    for (unsigned i = 0; i < 80; ++i)
    {
        strcat(source, "echo boundary-crossing\n");
        strcat(expected, "boundary-crossing\n");
    }
    CHECK(script(source, 0, 0) == 0);
    CHECK(!strcmp(output, expected));
    CHECK(read_calls == (strlen(source) + 511) / 512 + 1);
    CHECK(script(source, 7, 0) == 0);
    CHECK(!strcmp(output, expected));

    char name[] = "shell", option[] = "-c", command[] = "echo command";
    char *argv[] = {name, option, command, NULL};
    prepare("", 0, 0);
    CHECK(shell_main(3, argv) == 0);
    CHECK(!strcmp(output, "command\n") && read_calls == 0);
    CHECK(script("echo first && echo second || echo skipped; echo last\n", 0, 0) == 0);
    CHECK(!strcmp(output, "first\nsecond\nlast\n"));
    CHECK(script("missing && echo skipped || echo recovered && echo last\n", 0, 0) == 0);
    CHECK(!strcmp(output, "recovered\nlast\n"));
    CHECK(!strcmp(errors, "missing: command not found\n"));
    CHECK(script("echo 'a; b && c || d'\n", 0, 0) == 0);
    CHECK(!strcmp(output, "a; b && c || d\n"));
    CHECK(script("echo broken &&\n", 0, 0) == 2);
    CHECK(failures == 1 && !output_bytes);
#if defined(LUME_ARCH_AARCH64)
    char make_flags[610] = "MAKEFLAGS=";
    memset(make_flags + 10, 'x', 599);
    make_flags[609] = 0;
    char *make_environment[] = {make_flags, NULL};
    char inherited_command[] = "echo inherited";
    char *inherited_argv[] = {name, option, inherited_command, NULL};
    prepare("", 0, 0);
    shell_environment = make_environment;
    CHECK(shell_main(3, inherited_argv) == 0);
    CHECK(!strcmp(output, "inherited\n"));
    CHECK(!strcmp(value("MAKEFLAGS"), make_flags + 10));
#endif
    char expanded[LUME_ARG_BYTES], *arguments[LUME_ARGS + SHELL_STAGES];
    struct shell_command commands[SHELL_STAGES];
    struct shell_redirect redirects[LUME_ARGS];
    CHECK(shell_parse("echo 'a|b'|cat<in>>out", expanded, arguments, commands, redirects) == 2);
    CHECK(commands[0].argc == 2 && !strcmp(commands[0].argv[1], "a|b"));
    CHECK(commands[1].argc == 1 && commands[1].redirects == 2);
    CHECK(redirects[0].type == '<' && !strcmp(redirects[0].path, "in"));
    CHECK(redirects[1].type == 'a' && !strcmp(redirects[1].path, "out"));
    CHECK(shell_parse("echo x 2>&1", expanded, arguments, commands, redirects) == 1);
    CHECK(redirects[0].type == 'd' && redirects[0].fd == 2 && !strcmp(redirects[0].path, "1"));
    CHECK(shell_parse("echo 2>&1>file", expanded, arguments, commands, redirects) == 1);
    CHECK(commands[0].argc == 1 && commands[0].redirects == 2);
    CHECK(redirects[0].type == 'd' && !strcmp(redirects[0].path, "1"));
    CHECK(redirects[1].type == '>' && redirects[1].fd == 1 && !strcmp(redirects[1].path, "file"));
    CHECK(shell_parse("echo \\> \\| '2>' \"<\"", expanded, arguments, commands, redirects) == 1);
    CHECK(commands[0].argc == 5 && !commands[0].redirects);
    CHECK(!strcmp(commands[0].argv[1], ">") && !strcmp(commands[0].argv[2], "|"));
    CHECK(shell_parse("|cat", expanded, arguments, commands, redirects) == -22);
    CHECK(shell_parse("echo x|", expanded, arguments, commands, redirects) == -22);
    CHECK(shell_parse("echo x||cat", expanded, arguments, commands, redirects) == -22);
    CHECK(shell_parse("cat <", expanded, arguments, commands, redirects) == -22);
    CHECK(shell_parse("cat > | cat", expanded, arguments, commands, redirects) == -22);
    CHECK(shell_parse("echo x 16>out", expanded, arguments, commands, redirects) == -22);
    char snapshot[] = "05:FOO=x15:BAR=y";
    CHECK(shell_import(snapshot) == 0);
    CHECK(!strcmp(value("FOO"), "x") && !find("FOO")->exported);
    CHECK(!strcmp(value("BAR"), "y") && find("BAR")->exported);
    char invalid_snapshot[] = "099:FOO=x";
    CHECK(shell_import(invalid_snapshot) == -22);
    printf("PASS_SHELL_SCRIPT checks=%u\n", checks);
    return 0;
}
