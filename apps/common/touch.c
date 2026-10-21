#include "api.h"

int app_main(int argc, char **argv)
{
    if (argc < 2)
    {
        app_puts("usage: touch FILE...\n");
        return 2;
    }
    int failed = 0;
    for (int i = 1; i < argc; ++i)
    {
        struct lume_stat stat;
        if (app_stat(argv[i], &stat) == 0)
        {
            app_puts("touch: timestamp updates unavailable\n");
            failed = 1;
            continue;
        }
        int fd = app_open(argv[i], LUME_WRONLY | LUME_CREAT | LUME_EXCL);
        if (fd < 0)
        {
            app_error("touch", fd);
            failed = 1;
        }
        else if (app_close(fd) < 0)
            failed = 1;
    }
    return failed;
}
