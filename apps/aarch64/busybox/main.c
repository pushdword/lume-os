/* SPDX-License-Identifier: GPL-2.0-only */
#include "libbb.h"
#define APPLETS(X)                                                                                 \
    X(cat)                                                                                         \
    X(echo)                                                                                        \
    X(false)                                                                                       \
    X(true)                                                                                        \
    X(pwd)                                                                                         \
    X(printenv)                                                                                    \
    X(basename)                                                                                    \
    X(dirname)                                                                                     \
    X(head)                                                                                        \
    X(wc)                                                                                          \
    X(mkdir)                                                                                       \
    X(rmdir)                                                                                       \
    X(ls)                                                                                          \
    X(cp)                                                                                          \
    X(rm)                                                                                          \
    X(touch)
#define DECLARE(n) int n##_main(int, char **);
APPLETS(DECLARE)

struct applet
{
    const char *name;
    int (*run)(int, char **);
};

#define ENTRY(n) {#n, n##_main},
static const struct applet applets[] = {APPLETS(ENTRY)};

int app_main(int argc, char **argv)
{
    const char *name = strrchr(argv[0], '/');
    name = name ? name + 1 : argv[0];
    if (!strcmp(name, "busybox"))
    {
        if (argc < 2 || !strcmp(argv[1], "--list") || !strcmp(argv[1], "--help"))
        {
            puts("BusyBox 1.38.0 selected applets + Lume tools / AArch64");
            for (unsigned i = 0; i < sizeof(applets) / sizeof(applets[0]); ++i)
                puts(applets[i].name);
            return 0;
        }
        --argc;
        ++argv;
        name = argv[0];
    }
    applet_name = name;
    optind = 0;
    opterr = 0;
    for (unsigned i = 0; i < sizeof(applets) / sizeof(applets[0]); ++i)
        if (!strcmp(name, applets[i].name))
            return applets[i].run(argc, argv);
    bb_error_msg("unknown applet");
    return 127;
}
