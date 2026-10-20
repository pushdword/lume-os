#include "api.h"
static volatile uint32_t work;

int app_main(int argc, char **argv)
{
    uint32_t duration = 0;
    if (argc > 2)
        return 2;
    if (argc == 2)
    {
        const char *p = argv[1];
        if (!*p)
            return 2;
        while (*p)
        {
            if (*p < '0' || *p > '9' || duration > 3000)
                return 2;
            duration = duration * 10u + (unsigned)(*p++ - '0');
        }
        if (!duration || duration > 30000)
            return 2;
    }
    uint32_t start = (uint32_t)lume_syscall(LUME_INFO, LUME_UPTIME, 0, 0);
    /* Deliberately CPU-bound: progress of the shell depends on preemption,
     * rather than on a cooperative yield from these two workers. */
    for (;;)
    {
        work++;
        if (duration && !(work & 4095u) &&
            (uint32_t)((uint32_t)lume_syscall(LUME_INFO, LUME_UPTIME, 0, 0) - start) >= duration)
            return 0;
    }
}
