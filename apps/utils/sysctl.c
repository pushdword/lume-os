#include "sysctl_config.h"

int app_main(int argc, char **argv)
{
    if (argc == 1 || (argc == 2 && !strcmp(argv[1], "-a")))
    {
        int failed = 0;
        for (unsigned i = 0; i < sizeof(sysctl_keys) / sizeof(sysctl_keys[0]); ++i)
            failed |= sysctl_value(sysctl_keys[i].key, NULL, 1);
        return failed;
    }
    if ((argc == 2 || argc == 3) && !strcmp(argv[1], "-p"))
        return lume_sysctl_load(argc == 3 ? argv[2] : "/data/etc/sysctl.conf", 0, 1);
    if (argc >= 3 && !strcmp(argv[1], "-w"))
    {
        int failed = 0;
        for (int i = 2; i < argc; ++i)
        {
            char line[128];
            if (strlen(argv[i]) >= sizeof(line))
                return 2;
            strcpy(line, argv[i]);
            failed |= sysctl_assignment(line, 1);
        }
        return failed;
    }
    if (argc == 2 && argv[1][0] != '-')
        return sysctl_value(argv[1], NULL, 1);
    app_puts("usage: sysctl [-a] | KEY | -w KEY=VALUE... | -p [FILE]\n");
    return 2;
}
