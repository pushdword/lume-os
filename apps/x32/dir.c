#include "api.h"

int app_main(int argc, char **argv)
{
    if (argc > 2)
    {
        app_puts("usage: dir [directory]\n");
        return 2;
    }
    const char *path = argc == 2 ? argv[1] : ".";
    uint32_t cursor = 0, count = 0;
    app_puts("Directory of ");
    app_puts(path);
    app_puts("\n");
    for (;;)
    {
        struct lume_directory_entry e;
        int n =
            lume_syscall(LUME_READDIR, (uint32_t)(uintptr_t)path, (uint32_t)(uintptr_t)&e, cursor);
        if (n < 0)
        {
            app_error("dir", n);
            return 1;
        }
        if (!n)
            break;
        cursor = (uint32_t)n;
        count++;
        app_puts(e.name);
        app_puts("  ");
        if (e.mode & 0x4000u)
            app_puts("<DIR>");
        else
            app_number(e.size);
        app_puts("\n");
    }
    app_number(count);
    app_puts(" entries\n");
    return 0;
}
