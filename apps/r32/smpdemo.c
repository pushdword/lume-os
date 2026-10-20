/* Explicit bounded CPU stress. Normal init does not burn both cores. */
#include "api.h"

int app_main(void)
{
    const char *path = "/usr/libexec/worker" LUME_APP_SUFFIX "", *args[] = {path, "8000"};
    int pid[2];
    unsigned masks[2] = {0}, done[2] = {0};
    for (unsigned n = 0; n < 2; n++)
    {
        pid[n] = app_spawn(path, 2, args);
        if (pid[n] < 0)
        {
            app_error("smpdemo", pid[n]);
            if (n)
                (void)app_wait(pid[0], 0);
            return 1;
        }
    }
    while (!done[0] || !done[1])
    {
        for (unsigned slot = 0; slot < 8; slot++)
        {
            struct lume_process_info info;
            if (lume_syscall(LUME_PS, slot, (uint32_t)(uintptr_t)&info, 0) > 0)
                for (unsigned n = 0; n < 2; n++)
                    if (info.pid == (unsigned)pid[n])
                    {
                        masks[n] |= info.cores;
                        if (info.state == LUME_ZOMBIE)
                            done[n] = 1;
                    }
        }
        lume_syscall(LUME_YIELD, 0, 0, 0);
    }
    for (unsigned n = 0; n < 2; n++)
    {
        struct lume_wait_status status;
        if (app_wait(pid[n], &status) != pid[n] || status.code)
            return 1;
    }
    app_puts("SMP demo: worker masks ");
    app_number(masks[0]);
    app_puts("/");
    app_number(masks[1]);
#ifdef LUME_ARCH_R32
    unsigned expected = (1u << (unsigned)lume_syscall(LUME_INFO, LUME_CORES, 0, 0)) - 1u;
    unsigned ok = masks[0] && masks[1] && ((masks[0] | masks[1]) == expected);
#else
    unsigned ok = masks[0] && masks[1] && ((masks[0] | masks[1]) == 3);
#endif
#ifdef LUME_ARCH_R32
    app_puts(ok ? "; configured cores PASS; workers stopped\n" : "; FAIL\n");
    return ok ? 0 : 1;
#else
    app_puts(ok ? "; both cores PASS; workers stopped\n" : "; FAIL\n");
    return ok ? 0 : 1;
#endif
}
