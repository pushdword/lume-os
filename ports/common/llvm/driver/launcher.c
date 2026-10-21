#include <spawn.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "api.h"

extern char **environ;

int app_main(unsigned argc, char **argv)
{
    static const struct
    {
        const char *name, *program;
    } tools[] = {{"cc", "clang"}, {"c++", "clang"}, {"clang++", "clang"},
                 {"ar", "llvm-ar"}, {"ranlib", "llvm-ar"}, {"ld", "ld.lld"},
                 {"objcopy", "llvm-objcopy"}, {"strip", "llvm-objcopy"}, {"as", "clang"},
                 {"cpp", "clang"}, {"nm", "llvm-nm"}, {"readelf", "llvm-readobj"}};
    const char *name = strrchr(argv[0], '/');
    name = name ? name + 1 : argv[0];
    const char *program = NULL;
    for (unsigned i = 0; i < sizeof(tools) / sizeof(tools[0]); ++i)
        if (!strcmp(name, tools[i].name))
            program = tools[i].program;
    if (!program)
    {
        fputs("tool: unsupported command name\n", stderr);
        return 2;
    }
    char *arguments[LUME_ARGS + 1];
    unsigned count = 0;
    arguments[count++] = argv[0];
    unsigned version = argc == 2 && !strcmp(argv[1], "--version");
    if (!version && (!strcmp(name, "c++") || !strcmp(name, "clang++")))
    {
        arguments[count++] = "--driver-mode=g++";
        arguments[count++] = "-fno-exceptions";
        arguments[count++] = "-fno-rtti";
        arguments[count++] = "-stdlib=libc++";
    }
    if (!version && !strcmp(name, "as"))
    {
        arguments[count++] = "-c";
        arguments[count++] = "-x";
        arguments[count++] = "assembler";
    }
    if (!version && !strcmp(name, "cpp"))
        arguments[count++] = "-E";
    if (argc > LUME_ARGS - count + 1)
    {
        fputs("tool: too many arguments\n", stderr);
        return 2;
    }
    for (unsigned i = 1; i < argc; ++i)
        arguments[count++] = argv[i];
    arguments[count] = NULL;
    pid_t child;
    int result = posix_spawnp(&child, program, NULL, NULL, arguments, environ);
    if (result)
    {
        errno = result;
        perror(name);
        return 127;
    }
    int status;
    if (waitpid(child, &status, 0) != child)
    {
        perror(name);
        return 1;
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}
