#include "api.h"
int app_main(void)
{
    if (lume_syscall(LUME_WRITE, 0xf0001010u, 4, 0) != -14)
    {
        app_puts("PROTECTION FAILURE: invalid syscall pointer accepted\n");
        return 1;
    }
    app_puts("Probe: invalid syscall pointer rejected\n");
    app_puts("Probe: user mode attempts MMIO; expect page fault\n");
    *(volatile uint32_t *)(uintptr_t)0xf0001010u = 0;
    app_puts("PROTECTION FAILURE: user accessed MMIO\n");
    return 1;
}
