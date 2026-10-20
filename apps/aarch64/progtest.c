#include "lume_libc.h"
static volatile unsigned witness;

int app_main(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "fault"))
    {
        *(volatile unsigned *)(uintptr_t)0x200000 = 1;
        return 99;
    }
    if (argc != 3 || strcmp(argv[1], "argument with spaces") || strcmp(argv[2], "olá"))
        return 41;
    if (witness)
        return 46;
    witness = 1;
    char formatted[32];
    if (snprintf(formatted, sizeof(formatted), "%7llu", 42ULL) != 7 || strcmp(formatted, "     42"))
        return 47;
    if (snprintf(formatted, sizeof(formatted), "%.*s:%ld", 2, "hello", -7L) != 5 ||
        strcmp(formatted, "he:-7"))
        return 48;
    char cwd[128];
    if (!getcwd(cwd, sizeof(cwd)) || strcmp(cwd, "/tmp"))
        return 42;
    if (lume_call(LUME_SYS_GETPID, 0, 0, 0) != 2)
        return 43;
    if (lume_call(LUME_SYS_RUN, 0, 0, 0) != -LUME_EPERM)
        return 44;
    if (lume_call(0xffff, 0, 0, 0) != -LUME_ENOSYS)
        return 45;
    puts("LUME EXTERNAL ARGV CWD ISOLATION PASS");
    return 7;
}
