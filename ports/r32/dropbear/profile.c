#include "profile.h"
#ifdef LUME_SSH_PROFILE
#include "api.h"
#include "io.h"
#include <stdio.h>
static unsigned started, active, kernel_costs, profile_pid;
static struct lume_io_syscall_costs costs;

static unsigned profile_clock(void)
{
    return (unsigned)lume_syscall(LUME_INFO, LUME_UPTIME, 0, 0);
}
#ifdef LUME_SSH_PROFILE_PHASES
static unsigned elapsed[SSH_P_COUNT], calls[SSH_P_COUNT];

unsigned ssh_profile_now(void)
{
    return profile_clock();
}

void ssh_profile_record(unsigned kind, unsigned n)
{
    if (active && kind < SSH_P_COUNT)
    {
        elapsed[kind] += n;
        calls[kind]++;
    }
}
#endif
void ssh_profile_reset(void)
{
#ifdef LUME_SSH_PROFILE_PHASES
    for (unsigned i = 0; i < SSH_P_COUNT; i++)
    {
        elapsed[i] = 0;
        calls[i] = 0;
    }
#endif
    profile_pid = (unsigned)lume_syscall(LUME_PID, 0, 0, 0);
    /* Collect inside the kernel; no extra INFO calls per measured syscall.
     * Old kernels report this optional capability as unavailable. */
    kernel_costs = lume_syscall(LUME_IO, 5, 1, profile_pid) == 0;
    started = profile_clock();
    active = 1;
}

void ssh_profile_print(void)
{
    if (!active)
        return;
    unsigned duration = profile_clock() - started;
    active = 0;
    if (kernel_costs)
    {
        int stopped = lume_syscall(LUME_IO, 5, 0, 0);
        kernel_costs = stopped == 0 &&
                       lume_syscall(LUME_IO, 4, (uint32_t)(uintptr_t)&costs, sizeof costs) == 0;
        if (kernel_costs &&
            (costs.version != 1 || costs.bytes != sizeof costs || costs.count != 41 ||
             !costs.clock_hz || costs.enabled || costs.pid_filter != profile_pid))
            kernel_costs = 0;
    }
    printf("SSH_PROFILE session_ms=%u kernel_costs=%s phase_instrumentation=%u\n", duration,
           kernel_costs ? "available" : "unavailable",
#ifdef LUME_SSH_PROFILE_PHASES
           1u
#else
           0u
#endif
    );
#ifdef LUME_SSH_PROFILE_PHASES
    static const char *names[] = {"sha256", "sha512", "scalar", "aes",  "curve",
                                  "rng",    "select", "sleep",  "netio"};
    for (unsigned i = 0; i < SSH_P_COUNT; i++)
        printf("SSH_PHASE %s ms=%u calls=%u\n", names[i], elapsed[i], calls[i]);
#endif
    if (kernel_costs)
    {
        printf(
            "SSH_SYSCALL_SCOPE work_ticks_exclude_suspension include_device exclude_trap_dispatch clock_hz=%u pid=%u\n",
            costs.clock_hz, costs.pid_filter);
        for (unsigned i = 0; i < 41; i++)
            if (costs.syscall[i].calls)
            {
                const struct lume_io_syscall_cost *row = costs.syscall + i;
                printf(
                    "SSH_SYSCALL op=%u calls=%u errors=%u bytes=%u ticks_high=%u ticks_low=%u max_ticks=%u\n",
                    i, row->calls, row->errors, row->bytes, row->ticks_high, row->ticks_low,
                    row->max_ticks);
            }
    }
    fflush(stdout);
}
#endif
