/* SPDX-License-Identifier: BSD-3-Clause */
#include <errno.h>
#include <poll.h>
#include <stddef.h>

int app_main(unsigned argc, char **argv, char **environment)
{
    (void)argc;
    (void)argv;
    (void)environment;
    for (;;)
        if (poll(NULL, 0, 60000) < 0 && errno != EINTR)
            return 1;
}
