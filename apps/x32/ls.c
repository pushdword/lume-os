#include "api.h"

int app_main(int argc, char **argv)
{
    const char *path = ".";
    unsigned detailed = 0, seen = 0;
    for (int i = 1; i < argc; i++)
    {
        if (app_equal(argv[i], "-l"))
            detailed = 1;
        else if (!seen)
        {
            path = argv[i];
            seen = 1;
        }
        else
        {
            app_puts("usage: ls [-l] [directory]\n");
            return 2;
        }
    }
    uint32_t cursor = 0;
    for (;;)
    {
        struct lume_directory_entry e;
        int n =
            lume_syscall(LUME_READDIR, (uint32_t)(uintptr_t)path, (uint32_t)(uintptr_t)&e, cursor);
        if (n < 0)
        {
            app_error("ls", n);
            return 1;
        }
        if (!n)
            return 0;
        cursor = (uint32_t)n;
        if (detailed)
        {
            app_puts((e.mode & 0x4000u) ? "drwxr-xr-x root root "
                     : (e.mode & 0200u) ? "-rw-r--r-- root root "
                                        : "-r--r--r-- root root ");
            app_number(e.size);
            app_puts(" ");
        }
        app_puts(e.name);
        if (e.mode & 0x4000u)
            app_puts("/");
        app_puts("\n");
    }
}
