/* SPDX-License-Identifier: BSD-3-Clause */
#include <poll.h>
#include <stddef.h>

int pause(void)
{
    for (;;)
    {
        if (poll(NULL, 0, -1) < 0)
            return -1;
    }
}
