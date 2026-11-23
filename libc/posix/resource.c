/* SPDX-License-Identifier: BSD-3-Clause */
#include "native_posix.h"
#include <perf.h>
#include <pthread.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>

static int priority_target(int which, id_t who)
{
    switch (which)
    {
    case PRIO_PROCESS:
        return !who || who == (id_t)getpid();
    case PRIO_PGRP:
        return !who || who == (id_t)getpgrp();
    case PRIO_USER:
        return !who || who == (id_t)getuid();
    default:
        errno = EINVAL;
        return -1;
    }
}

int getpriority(int which, id_t who)
{
    int matches = priority_target(which, who);
    if (matches < 0)
        return -1;
    if (!matches)
    {
        errno = ESRCH;
        return -1;
    }
    return 0;
}

int setpriority(int which, id_t who, int priority)
{
    int matches = priority_target(which, who);
    if (matches < 0)
        return -1;
    if (!matches)
    {
        errno = ESRCH;
        return -1;
    }
    if (priority < -20 || priority > 19)
    {
        errno = EINVAL;
        return -1;
    }
    if (priority)
    {
        errno = ENOTSUP;
        return -1;
    }
    return 0;
}

static void usage_time(uint32_t ticks, uint32_t frequency, struct timeval *out)
{
    out->tv_sec = ticks / frequency;
    out->tv_usec = (suseconds_t)(((uint64_t)(ticks % frequency) * 1000000u) / frequency);
}

static int usage_matches(const struct lume_perf_process *process, uint32_t target, int thread)
{
    if (process->pid != target)
        return 0;
    return thread ? ((process->state & LUME_PERF_THREAD) || process->pid == (uint32_t)getpid())
                  : !(process->state & LUME_PERF_THREAD);
}

static const struct lume_perf_process *snapshot_process(const struct lume_perf_snapshot *snapshot,
                                                        uint32_t target, int thread)
{
    for (uint32_t index = 0; index < snapshot->process_count; ++index)
        if (usage_matches(&snapshot->processes[index], target, thread))
            return &snapshot->processes[index];
    return NULL;
}

static int page_process(uint32_t target, int thread, struct lume_perf_process *out)
{
    uint32_t cursor = 0;
    do
    {
        struct lume_perf_task_page page = {.cursor = cursor};
        int64_t result = native_call(LUME_SYS_TELEMETRY, (uintptr_t)&page, sizeof(page),
                                     LUME_PERF_TASK_PAGE);
        if (result < 0)
            return native_result(result);
        if (page.count > LUME_PERF_PAGE_PROCESSES || page.next == cursor)
        {
            errno = EIO;
            return -1;
        }
        for (uint32_t index = 0; index < page.count; ++index)
            if (usage_matches(&page.processes[index], target, thread))
            {
                *out = page.processes[index];
                return 0;
            }
        cursor = page.next;
    } while (cursor);
    errno = ESRCH;
    return -1;
}

int getrusage(int who, struct rusage *usage)
{
    if (!usage)
    {
        errno = EFAULT;
        return -1;
    }
    if (who == RUSAGE_CHILDREN)
    {
        errno = ENOSYS;
        return -1;
    }
    if (who != RUSAGE_SELF && who != RUSAGE_THREAD)
    {
        errno = EINVAL;
        return -1;
    }

    struct lume_perf_snapshot snapshot;
    int64_t result = native_call(LUME_SYS_TELEMETRY, (uintptr_t)&snapshot, sizeof(snapshot),
                                 LUME_PERF_VERSION);
    if (result < 0)
        return native_result(result);
    if (snapshot.version != LUME_PERF_VERSION || snapshot.bytes != sizeof(snapshot) ||
        !snapshot.clock_hz || snapshot.process_count > LUME_PERF_PROCESSES)
    {
        errno = EIO;
        return -1;
    }

    int thread = who == RUSAGE_THREAD;
    uint32_t target = thread ? (uint32_t)pthread_self() : (uint32_t)getpid();
    struct lume_perf_process process;
    const struct lume_perf_process *found = snapshot_process(&snapshot, target, thread);
    if (found)
        process = *found;
    else if (page_process(target, thread, &process))
        return -1;

    memset(usage, 0, sizeof(*usage));
    usage_time(process.user_ticks, snapshot.clock_hz, &usage->ru_utime);
    usage_time(process.kernel_ticks, snapshot.clock_hz, &usage->ru_stime);
    return 0;
}
