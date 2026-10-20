/* SPDX-License-Identifier: GPL-2.0-only */
#include "libbb.h"
#include "api.h"

struct applet
{
    const char *name;
    int (*main)(int, char **);
};

#define DECLARE(n) int n##_main(int, char **)
DECLARE(basename);
DECLARE(cat);
DECLARE(dirname);
DECLARE(echo);
DECLARE(false);
DECLARE(head);
DECLARE(printenv);
DECLARE(pwd);
DECLARE(true);
DECLARE(wc);
DECLARE(sort);
DECLARE(cut);
DECLARE(cmp);
DECLARE(cksum);
DECLARE(expand);
DECLARE(mkdir);
DECLARE(rmdir);
static const struct applet applets[] = {{"basename", basename_main}, {"cat", cat_main},
                                        {"dirname", dirname_main},   {"echo", echo_main},
                                        {"false", false_main},       {"head", head_main},
                                        {"printenv", printenv_main}, {"pwd", pwd_main},
                                        {"true", true_main},         {"wc", wc_main},
                                        {"sort", sort_main},         {"cut", cut_main},
                                        {"cmp", cmp_main},           {"cksum", cksum_main},
                                        {"crc32", cksum_main},       {"expand", expand_main},
                                        {"unexpand", expand_main},   {"mkdir", mkdir_main},
                                        {"rmdir", rmdir_main}};

int app_main(int argc, char **argv)
{
    const char *p = strrchr(argv[0], '/');
    p = p ? p + 1 : argv[0];
    char name[16];
    size_t n = strlen(p);
    if (n > 4 && !strcmp(p + n - 4, LUME_APP_SUFFIX))
        n -= 4;
    if (n >= sizeof name)
        return 127;
    memcpy(name, p, n);
    name[n] = 0;
    if (!strcmp(name, "busybox"))
    {
        if (argc >= 2 && !strcmp(argv[1], "--list"))
        {
            for (unsigned i = 0; i < sizeof applets / sizeof applets[0]; i++)
                puts(applets[i].name);
            return 0;
        }
        if (argc < 2 || !strcmp(argv[1], "--help"))
        {
            puts("BusyBox 1.38.0 / LumeOS selected-applet port (GPLv2)");
            puts("basename [-a|-s SUFFIX] FILE [SUFFIX]; dirname FILE");
            puts("cat [-u] [FILE...]; echo [-neE] [ARG...]; head [-n N] [FILE...]");
            puts("printenv [NAME...]; pwd; true; false; wc [-lwmcL] [FILE...]");
            puts("sort [-rus] [FILE...]; cut -b/-c/-f LIST [-d CHAR] [FILE...]");
            puts("cmp [-s|-l] [-n N] FILE1 [FILE2]; cksum/crc32 [FILE...]");
            puts("expand [-i] [-t N] [FILE...]; unexpand [-fa] [-t N] [FILE...]");
            puts("mkdir [-pv] DIR...; rmdir [-pv] DIR... (FAT synthetic modes)");
            return 0;
        }
        p = argv[1];
        argv++;
        argc--;
    }
    else
        p = name;
    applet_name = p;
    for (unsigned i = 0; i < sizeof applets / sizeof applets[0]; i++)
        if (!strcmp(p, applets[i].name))
        {
            /* Picolibc resets its internal option cursor when optind is zero. */
            optind = 0;
            opterr = 0;
            return applets[i].main(argc, argv);
        }
    bb_error_msg("applet unavailable");
    return 127;
}
