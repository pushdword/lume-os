/* Exercise the actual ring3 libc and kernel boundary, not host substitutions. */
#include "api.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
extern char **environ;
#ifndef LUME_TEST_HEAP_EXCESS
#define LUME_TEST_HEAP_EXCESS 131072
#endif
#define CHECK(c)                                                                                   \
    do                                                                                             \
    {                                                                                              \
        if (!(c))                                                                                  \
        {                                                                                          \
            printf("LIBCTEST FAIL line=%u errno=%d\n", __LINE__, errno);                           \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)

static uint32_t ptr(const void *p)
{
    return (uint32_t)(uintptr_t)p;
}

int app_main(int argc, char **argv, char **envp)
{
    CHECK(envp == environ && argv[argc] == NULL);
    CHECK(lume_syscall(LUME_WRITE, 0, 0, 0) == 0);
    CHECK(app_write(1, NULL, 0) == 0);
    if (argc == 2)
    {
        CHECK(!strcmp(argv[1], "child"));
        CHECK(getenv("SNAPSHOT") && !strcmp(getenv("SNAPSHOT"), "parent-before"));
        CHECK(!getenv("PRIVATE"));
        CHECK(setenv("SNAPSHOT", "child-only", 1) == 0);
        puts("LIBCTEST CHILD: environment snapshot/private heap PASS");
        return 0;
    }
    CHECK(getenv("USER") && !strcmp(getenv("USER"), "root"));
    void *huge = malloc(LUME_TEST_HEAP_EXCESS);
    CHECK(!huge && errno == ENOMEM);
    unsigned char *buffer = malloc(8192);
    CHECK(buffer);
    for (unsigned i = 0; i < 8192; i++)
        buffer[i] = (unsigned char)(i * 17u + 3u);
    buffer = realloc(buffer, 16384);
    CHECK(buffer);
    for (unsigned i = 0; i < 8192; i++)
        CHECK(buffer[i] == (unsigned char)(i * 17u + 3u));
    free(buffer);
    (void)mkdir("/tmp", 0755);
    FILE *f = fopen("/tmp/libc.txt", "w+");
    CHECK(f);
    for (unsigned i = 0; i < 80; i++)
        CHECK(fprintf(f, "line %02u hello\n", i) > 0);
    CHECK(fflush(f) == 0 && fseek(f, 0, SEEK_SET) == 0);
    char text[32];
    for (unsigned i = 0; i < 80; i++)
    {
        CHECK(fgets(text, sizeof text, f));
        CHECK(!strncmp(text, "line ", 5));
    }
    CHECK(fgetc(f) == EOF && !ferror(f) && fclose(f) == 0);
    errno = 0;
    CHECK(!fopen("/tmp/absent.zzz", "r") && errno == ENOENT);
    char cwd[128];
    CHECK(getcwd(cwd, sizeof cwd) && chdir("/tmp") == 0);
    CHECK(getcwd(NULL, 128) != NULL);
    CHECK(chdir(cwd) == 0);
    const char *child[] = {"libctest", "child"};
    const char *env[] = {"SNAPSHOT=parent-before"};
    CHECK(setenv("PRIVATE", "parent", 1) == 0);
    int pid = app_spawn_env("/usr/libexec/libctest" LUME_APP_SUFFIX "", 2, child, 1, env);
    CHECK(pid >= 0);
    CHECK(setenv("SNAPSHOT", "parent-after", 1) == 0);
    struct lume_wait_status status;
    CHECK(app_wait(pid, &status) == pid && status.code == 0);
    CHECK(!strcmp(getenv("SNAPSHOT"), "parent-after"));
    struct lume_spawn_request request = {ptr("/bin/true" LUME_APP_SUFFIX ""), 1, ptr(child), 1,
                                         0xf0000000u};
    CHECK(lume_syscall(LUME_SPAWN_ENV, ptr(&request), 0, 0) == -14);
    request.envc = LUME_ENVS + 1;
    CHECK(lume_syscall(LUME_SPAWN_ENV, ptr(&request), 0, 0) == -22);
    char too_long[LUME_ENV_BYTES + 1];
    memset(too_long, 'a', sizeof too_long - 1);
    too_long[sizeof too_long - 1] = 0;
    const char *longenv[] = {too_long};
    request.envc = 1;
    request.envp = ptr(longenv);
    CHECK(lume_syscall(LUME_SPAWN_ENV, ptr(&request), 0, 0) == -7);
    CHECK(lume_syscall(LUME_SPAWN_ENV, 0xf0000000u, 0, 0) == -14);
    CHECK(lume_syscall(LUME_READ, 0, 0x40018000u, 1) == -14); /* guard page */
    CHECK(lume_syscall(LUME_READ, 0, 0x40000000u, 1) == -14); /* RX page */
    puts("LIBCTEST PASS: heap/stdio/errno/argv/env/private pages/bounds/wait");
    return 0;
}
