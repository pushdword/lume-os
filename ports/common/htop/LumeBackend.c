/* SPDX-License-Identifier: GPL-2.0-or-later
 * Native platform backend for htop3.5.3. No procfs or invented task samples. */
#include "config.h"
#include "Platform.h"
#include "unsupported/UnsupportedMachine.h"
#include "unsupported/UnsupportedProcessTable.h"
#include "unsupported/UnsupportedProcess.h"
#include "CPUMeter.h"
#include "TasksMeter.h"
#include "UptimeMeter.h"
#include "HostnameMeter.h"
#include "SysArchMeter.h"
#include "api.h"
#include "perf.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
static struct lume_perf_snapshot sample, previous;
static struct lume_memory_info memory;
static bool valid, have_previous, memory_valid;

static uint32_t delta(uint32_t a, uint32_t b)
{
    return a - b;
}

const ScreenDefaults Platform_defaultScreens[] = {
    {.name = "Lume",
     .columns = "PID USER STATE PERCENT_CPU TIME Command",
     .sortKey = "PERCENT_CPU"}};
const unsigned Platform_numberOfDefaultScreens = 1;
const SignalItem Platform_signals[] = {{.name = "0 Cancel", .number = 0}};
const unsigned Platform_numberOfSignals = 1;
const MemoryClass Platform_memoryClasses[] = {
    {.label = "allocated pool", .countsAsUsed = true, .countsAsCache = false, .color = MEMORY_1}};
const unsigned Platform_numberOfMemoryClasses = 1;
const MeterClass *const Platform_meterTypes[] = {&CPUMeter_class,
                                                 &AllCPUsMeter_class,
                                                 &AllCPUs2Meter_class,
                                                 &LeftCPUsMeter_class,
                                                 &RightCPUsMeter_class,
                                                 &MemoryMeter_class,
                                                 &TasksMeter_class,
                                                 &HostnameMeter_class,
                                                 &SysArchMeter_class,
                                                 &UptimeMeter_class,
                                                 &SecondsUptimeMeter_class,
                                                 &BlankMeter_class,
                                                 NULL};

bool Platform_init(void)
{
    Settings_enableReadonly();
    return true;
}

void Platform_done(void)
{
}

void Platform_setBindings(Htop_Action *keys)
{
    const char *unavailable = "Mlsexa[]kt*+-=\177";
    for (const char *p = unavailable; *p; p++)
        if (*p != ' ')
            keys[(unsigned char)*p] = NULL;
    keys[KEY_F(7)] = keys[KEY_F(8)] = keys[KEY_F(9)] = NULL;
    keys[KEY_F(5)] = NULL; /* Upstream tree sentinel PID0 conflicts with Lume init. */
}

int Platform_getUptime(void)
{
    return (int)((unsigned)lume_syscall(LUME_INFO, LUME_UPTIME, 0, 0) / 1000u);
}

void Platform_getLoadAverage(double *a, double *b, double *c)
{
    *a = *b = *c = NAN;
}

pid_t Platform_getMaxPid(void)
{
    return INT_MAX;
}

char *Platform_getProcessEnv(pid_t p)
{
    (void)p;
    return NULL;
}

FileLocks_ProcessData *Platform_getProcessLocks(pid_t p)
{
    (void)p;
    return NULL;
}

void Platform_getFileDescriptors(double *a, double *b)
{
    *a = *b = NAN;
}

bool Platform_getDiskIO(DiskIOData *p)
{
    (void)p;
    return false;
}

bool Platform_getNetworkIO(NetworkIOData *p)
{
    (void)p;
    return false;
}

void Platform_getBattery(double *p, ACPresence *a)
{
    *p = NAN;
    *a = AC_ERROR;
}

void Platform_getHostname(char *b, size_t n)
{
    String_safeStrncpy(b, "Lume " LUME_HTOP_MACHINE, n);
}

const char *Platform_getRelease(void)
{
    return "Lume " LUME_HTOP_MACHINE " / read-only native backend";
}

void Platform_setSwapValues(Meter *m)
{
    (void)m;
}

void Platform_setMemoryValues(Meter *m)
{
    m->total = memory_valid ? (double)memory.total_pages * memory.page_bytes / 1024.0 : NAN;
    m->values[0] =
        memory_valid ? (double)(memory.total_pages - memory.free_pages) * memory.page_bytes / 1024.0
                     : NAN;
    m->curItems = 1;
}

double Platform_setCPUValues(Meter *m, unsigned cpu)
{
    double *v = m->values;
    for (unsigned i = 0; i < CPU_METER_ITEMCOUNT; i++)
        v[i] = 0;
    v[CPU_METER_FREQUENCY] = v[CPU_METER_TEMPERATURE] = NAN;
    m->curItems = 4;
    if (!valid || !have_previous)
        return NAN;
    uint64_t totals[4] = {0, 0, 0, 0};
    for (unsigned c = 0; c < sample.core_count; c++)
        if (cpu == 0 || c + 1 == cpu)
            for (unsigned k = 0; k < 4; k++)
                totals[k] += delta(sample.cores[c].ticks[k], previous.cores[c].ticks[k]);
    uint64_t all = totals[0] + totals[1] + totals[2] + totals[3];
    if (!all)
        return 0;
    v[CPU_METER_NORMAL] = (double)totals[LUME_PERF_USER] * 100.0 / all;
    /* Kernel spin/lock work consumesCPU and is included in system time. */
    v[CPU_METER_KERNEL] =
        (double)(totals[LUME_PERF_KERNEL] + totals[LUME_PERF_LOCKWAIT]) * 100.0 / all;
    return v[CPU_METER_NORMAL] + v[CPU_METER_KERNEL];
}

Machine *Machine_new(UsersTable *users, uid_t uid)
{
    UnsupportedMachine *p = xCalloc(1, sizeof *p);
    Machine *m = &p->super;
    Machine_init(m, users, uid);
    m->existingCPUs = m->activeCPUs = (unsigned)lume_syscall(LUME_INFO, LUME_CORES, 0, 0);
    return m;
}

void Machine_delete(Machine *m)
{
    Machine_done(m);
    free(m);
}

bool Machine_isCPUonline(const Machine *m, unsigned c)
{
    return c < m->activeCPUs;
}

int Machine_getCPUPhysicalCoreID(const Machine *m, unsigned c)
{
    (void)m;
    return (int)c;
}

int Machine_getCPUThreadIndex(const Machine *m, unsigned c)
{
    (void)m;
    (void)c;
    return 0;
}

void Machine_scan(Machine *m)
{
    /* Flat process lists preserve PID0. Generic htop's tree uses 0 as a root
  * sentinel, so a saved config must not re-enable that unsupported view. */
    if (m->settings)
        for (size_t s = 0; s < m->settings->nScreens; s++)
            m->settings->screens[s]->treeView = false;
    previous = sample;
    have_previous = valid;
    int n = lume_syscall(LUME_PERF, (uint32_t)(uintptr_t)&sample, sizeof sample, LUME_PERF_VERSION);
    valid = n == 0 && sample.version == 1 && sample.bytes == sizeof sample &&
            sample.core_count > 0 && sample.core_count <= 2 && sample.process_count <= 8 &&
            sample.clock_hz;
    if (!valid)
    {
        have_previous = false;
        errno = EIO;
        CRT_fatalError("Lume telemetry unavailable or incompatible");
    }
    for (unsigned i = 0; i < sample.process_count; i++)
        sample.processes[i].name[sizeof sample.processes[i].name - 1] = 0;
    if (have_previous &&
        (sample.core_count != previous.core_count || sample.clock_hz != previous.clock_hz ||
         !delta(sample.uptime_ms, previous.uptime_ms) ||
         (uint64_t)delta(sample.uptime_ms, previous.uptime_ms) * sample.clock_hz >=
             UINT64_C(4294967296) * 1000))
        have_previous = false;
    m->existingCPUs = m->activeCPUs = sample.core_count;
    memory_valid = lume_syscall(LUME_MEMORY, (uint32_t)(uintptr_t)&memory, sizeof memory,
                                LUME_MEMORY_VERSION) == 0 &&
                   memory.version == LUME_MEMORY_VERSION && memory.bytes == sizeof memory &&
                   memory.page_bytes && memory.free_pages <= memory.total_pages;
    m->totalMem = memory_valid ? (uint64_t)memory.total_pages * memory.page_bytes / 1024 : 0;
    m->totalSwap = m->usedSwap = m->cachedSwap = 0;
}

ProcessTable *ProcessTable_new(Machine *m, Hashtable *matches)
{
    UnsupportedProcessTable *t = xCalloc(1, sizeof *t);
    Object_setClass(t, Class(ProcessTable));
    ProcessTable_init(&t->super, Class(Process), m, matches);
    return &t->super;
}

void ProcessTable_delete(Object *p)
{
    UnsupportedProcessTable *t = (UnsupportedProcessTable *)p;
    ProcessTable_done(&t->super);
    free(t);
}

void ProcessTable_goThroughEntries(ProcessTable *t)
{
    if (!valid)
        return;
    uint32_t wall = have_previous ? delta(sample.cores[0].counter, previous.cores[0].counter) : 0;
    for (unsigned i = 0; i < sample.process_count; i++)
    {
        const struct lume_perf_process *a = &sample.processes[i], *b = NULL;
        if (have_previous)
            for (unsigned j = 0; j < previous.process_count; j++)
                if (previous.processes[j].pid == a->pid)
                    b = &previous.processes[j];
        bool exists;
        Process *p = ProcessTable_getProcess(t, (pid_t)a->pid, &exists, UnsupportedProcess_new);
        Process_setParent(p, a->parent == UINT32_MAX ? 0 : (pid_t)a->parent);
        Process_setThreadGroup(p, (pid_t)a->pid);
        Process_updateComm(p, a->name);
        Process_updateCmdline(p, a->name, 0, strlen(a->name));
        p->super.updated = true;
        p->super.show = true;
        p->isKernelThread = p->isUserlandThread = false;
        p->state = a->state == LUME_RUNNING  ? RUNNING
                   : a->state == LUME_READY  ? RUNNABLE
                   : a->state == LUME_ZOMBIE ? ZOMBIE
                                             : SLEEPING;
        p->st_uid = 0;
        p->user = "root";
        p->nice = PROCESS_NICE_UNKNOWN;
        p->nlwp = 1;
        uint64_t ticks = b ? (uint64_t)delta(a->user_ticks, b->user_ticks) +
                                 delta(a->kernel_ticks, b->kernel_ticks)
                           : 0;
        p->percent_cpu = b && wall ? (double)ticks * 100.0 / wall : NAN;
        p->percent_mem = NAN;
        UnsupportedProcess *lp = (UnsupportedProcess *)p;
        /* ABI716 has modulo counters, not a lifetime epoch. Never infer missed
   * wraps from a pre-existing observer session or a long sampling gap. */
        if (!exists)
            lp->lume_time_ticks = 0;
        else if (have_previous)
            lp->lume_time_ticks += ticks;
        p->time = lp->lume_time_ticks * 100u / sample.clock_hz;
        Process_updateCPUFieldWidths(p->percent_cpu);
        t->totalTasks++;
        if (a->state == LUME_RUNNING)
            t->runningTasks++;
        if (!exists)
            ProcessTable_add(t, p);
    }
}

/* htop uses monotonic time for refresh and highlighting. This profile does not provide a
 * wall-clock meter or CLOCK_REALTIME service. */
void Generic_gettime_realtime(struct timespec *t, uint64_t *ms)
{
    *ms = (uint32_t)lume_syscall(LUME_INFO, LUME_UPTIME, 0, 0);
    t->tv_sec = (time_t)(*ms / 1000);
    t->tv_nsec = (long)(*ms % 1000) * 1000000;
}

void Generic_gettime_monotonic(uint64_t *ms)
{
    *ms = (uint32_t)lume_syscall(LUME_INFO, LUME_UPTIME, 0, 0);
}
