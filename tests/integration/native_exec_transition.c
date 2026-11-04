#define _POSIX_C_SOURCE 200809L
#include "lume/abi.h"
#include "lume/syscall_native.h"
#ifdef LUME_EXEC_CHILD
int main(void)
{
    static const char text[] = "EXEC_CHILD_ENTRY_PASS\n";
    return lume_native_call(LUME_SYS_WRITE, 1, (uintptr_t)text, sizeof(text) - 1) ==
                   sizeof(text) - 1
               ? 0
               : 1;
}
#else
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    pid_t child = fork();
    if (child < 0)
        return 3;
    if (!child)
    {
        char *arguments[] = {argv[1], NULL};
        execv(argv[1], arguments);
        _exit(127);
    }
    int status = -1;
    if (waitpid(child, &status, 0) != child || status)
        return 4;
    puts("EXEC_CONTEXT_REPLACEMENT_PASS");
    return 0;
}
#endif
