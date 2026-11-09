/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/reboot.h>
#include <sys/syscall.h>
#endif

#define MEMORY_BYTES (4u * 1024u * 1024u)
#define NODES (256u * 1024u)
#define CPU_OPERATIONS (8u * 1024u * 1024u)
#define PID_OPERATIONS (64u * 1024u)

#ifndef BENCH_DEFAULT_SCALE
#define BENCH_DEFAULT_SCALE 1
#endif
#ifndef BENCH_DEFAULT_REPS
#define BENCH_DEFAULT_REPS 5
#endif
_Static_assert(BENCH_DEFAULT_SCALE >= 1 && BENCH_DEFAULT_SCALE <= 64, "work scale bounds");
_Static_assert(BENCH_DEFAULT_REPS >= 1 && BENCH_DEFAULT_REPS <= 9, "sample count bounds");

static unsigned scale = BENCH_DEFAULT_SCALE, repetitions = BENCH_DEFAULT_REPS;
static const char *selection = "all";
static uint64_t resolution = 1;
static volatile uint64_t observed;
static unsigned failed;
static void *(*volatile copy_bytes)(void *, const void *, size_t) = memcpy;
static void *(*volatile fill_bytes)(void *, int, size_t) = memset;

static int stamp(uint64_t *value)
{
    struct timespec time;
    if (clock_gettime(CLOCK_MONOTONIC, &time))
        return errno ? errno : EIO;
    *value = (uint64_t)time.tv_sec * UINT64_C(1000000000) + (unsigned long)time.tv_nsec;
    return 0;
}

static int unsupported(int error)
{
    return error == ENOSYS || error == ENOTSUP;
}

static void result(const char *name, unsigned workers, unsigned sample, uint64_t elapsed,
                   uint64_t operations, uint64_t bytes, uint64_t checksum, int error)
{
    const char *status = error                        ? (unsupported(error) ? "SKIP" : "ERROR")
                         : elapsed < resolution * 100 ? "TIMER_LIMIT"
                                                      : "OK";
    observed ^= checksum;
    if (error && !unsupported(error))
        ++failed;
    printf("BENCH,%s,%u,%s,%u,%u,%llu,%llu,%llu,%llu,%s,%d\n", name, workers,
           sample ? "sample" : "warmup", sample, scale, (unsigned long long)elapsed,
           (unsigned long long)operations, (unsigned long long)bytes, (unsigned long long)checksum,
           status, error);
    fflush(stdout);
}

static int selected(const char *name)
{
    return !strcmp(selection, "all") || !strcmp(selection, name);
}

static uint32_t mix(uint32_t value)
{
    value ^= value << 13;
    value ^= value >> 17;
    return value ^ (value << 5);
}

__attribute__((noinline)) static uint32_t cpu_loop(uint64_t count, uint32_t seed)
{
    for (uint64_t i = 0; i < count; ++i)
        seed = mix(seed) + UINT32_C(0x9e3779b9);
    return seed;
}

static long kernel_pid(void)
{
#ifdef __linux__
    return syscall(SYS_getpid);
#else
    return getpid();
#endif
}

static int pid_loop(uint64_t count, uint64_t *checksum)
{
    uint64_t sum = 0;
    for (uint64_t i = 0; i < count; ++i)
    {
        long pid = kernel_pid();
        if (pid <= 0)
            return errno ? errno : EIO;
        sum += (unsigned long)pid;
    }
    *checksum = sum;
    return 0;
}

static void cpu_case(void)
{
    uint64_t count = (uint64_t)CPU_OPERATIONS * scale;
    for (unsigned sample = 0; sample <= repetitions; ++sample)
    {
        uint64_t began = 0, ended = 0;
        int error = stamp(&began);
        uint64_t checksum = error ? 0 : cpu_loop(count, UINT32_C(0x12345678));
        if (!error)
            error = stamp(&ended);
        result("cpu_u32", 1, sample, ended >= began ? ended - began : 0, error ? 0 : count, 0,
               checksum, error);
    }
}

static void syscall_case(void)
{
    uint64_t count = (uint64_t)PID_OPERATIONS * scale;
    for (unsigned sample = 0; sample <= repetitions; ++sample)
    {
        uint64_t began = 0, ended = 0, checksum = 0;
        int error = stamp(&began);
        if (!error)
            error = pid_loop(count, &checksum);
        if (!error)
            error = stamp(&ended);
        result("getpid_kernel", 1, sample, ended >= began ? ended - began : 0, error ? 0 : count, 0,
               checksum, error);
        if (error)
            break;
    }
}

static uint64_t sum_bytes(const unsigned char *data, size_t bytes)
{
    uint64_t sum = 0;
    for (size_t i = 0; i < bytes; ++i)
        sum += data[i];
    return sum;
}

static void memory_case(int fill)
{
    const char *name = fill ? "memset_4m" : "memcpy_4m";
    unsigned char *source = malloc(MEMORY_BYTES), *target = malloc(MEMORY_BYTES);
    if (!source || !target)
    {
        result(name, 1, 0, 0, 0, 0, 0, ENOMEM);
        free(source);
        free(target);
        return;
    }
    for (size_t i = 0; i < MEMORY_BYTES; ++i)
        source[i] = (unsigned char)(i * 17u + i / 4096u);
    memset(target, 0, MEMORY_BYTES);
    unsigned count = 16 * scale;
    for (unsigned sample = 0; sample <= repetitions; ++sample)
    {
        uint64_t began = 0, ended = 0;
        int error = stamp(&began);
        if (!error)
        {
            for (unsigned i = 0; i < count; ++i)
                if (fill)
                    fill_bytes(target, (int)((i + 1) & 255), MEMORY_BYTES);
                else
                {
                    source[0] = (unsigned char)i;
                    copy_bytes(target, source, MEMORY_BYTES);
                }
        }
        if (!error)
            error = stamp(&ended);
        uint64_t checksum = sum_bytes(target, MEMORY_BYTES);
        if (!error && (fill ? checksum != (uint64_t)(count & 255) * MEMORY_BYTES
                            : memcmp(source, target, MEMORY_BYTES) != 0))
            error = EIO;
        result(name, 1, sample, ended >= began ? ended - began : 0, error ? 0 : count,
               error ? 0 : (uint64_t)count * MEMORY_BYTES, checksum, error);
    }
    free(source);
    free(target);
}

static void pointer_case(void)
{
    uint32_t *order = malloc((size_t)NODES * sizeof(*order));
    uint32_t *next = malloc((size_t)NODES * sizeof(*next));
    if (!order || !next)
    {
        result("pointer_chase_1m", 1, 0, 0, 0, 0, 0, ENOMEM);
        free(order);
        free(next);
        return;
    }
    for (unsigned i = 0; i < NODES; ++i)
        order[i] = i;
    uint32_t seed = UINT32_C(0x12345678);
    for (unsigned i = NODES - 1; i; --i)
    {
        seed = mix(seed);
        unsigned at = seed % (i + 1);
        uint32_t saved = order[i];
        order[i] = order[at];
        order[at] = saved;
    }
    for (unsigned i = 0; i < NODES; ++i)
        next[order[i]] = order[(i + 1) % NODES];
    volatile const uint32_t *links = next;
    uint64_t count = (uint64_t)NODES * 16 * scale;
    for (unsigned sample = 0; sample <= repetitions; ++sample)
    {
        uint32_t at = order[0];
        uint64_t began = 0, ended = 0;
        int error = stamp(&began);
        if (!error)
            for (uint64_t i = 0; i < count; ++i)
                at = links[at];
        if (!error)
            error = stamp(&ended);
        if (!error && at != order[0])
            error = EIO;
        result("pointer_chase_1m", 1, sample, ended >= began ? ended - began : 0, error ? 0 : count,
               error ? 0 : count * sizeof(*next), at, error);
    }
    free(order);
    free(next);
}

static int one_byte(int descriptor, unsigned char *value, int output)
{
    ssize_t bytes;
    do
        bytes = output ? write(descriptor, value, 1) : read(descriptor, value, 1);
    while (bytes < 0 && errno == EINTR);
    return bytes == 1 ? 0 : bytes < 0 && errno ? errno : EIO;
}

static void pipe_case(void)
{
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
    {
        result("pipe_pingpong", 2, 0, 0, 0, 0, 0, errno ? errno : EIO);
        return;
    }
    unsigned count = 512 * scale;
    for (unsigned sample = 0; sample <= repetitions; ++sample)
    {
        int outbound[2], inbound[2];
        if (pipe(outbound))
        {
            result("pipe_pingpong", 2, sample, 0, 0, 0, 0, errno);
            return;
        }
        if (pipe(inbound))
        {
            int error = errno;
            close(outbound[0]);
            close(outbound[1]);
            result("pipe_pingpong", 2, sample, 0, 0, 0, 0, error);
            return;
        }
        fflush(stdout);
        pid_t child = fork();
        if (!child)
        {
            close(outbound[1]);
            close(inbound[0]);
            unsigned char value;
            int error = 0;
            for (unsigned i = 0; i < count && !error; ++i)
            {
                error = one_byte(outbound[0], &value, 0);
                if (!error)
                    error = one_byte(inbound[1], &value, 1);
            }
            close(outbound[0]);
            close(inbound[1]);
            _exit(error ? 1 : 0);
        }
        int error = child < 0 ? errno : 0;
        close(outbound[0]);
        close(inbound[1]);
        uint64_t began = 0, ended = 0, checksum = 0;
        if (!error)
            error = stamp(&began);
        for (unsigned i = 0; i < count && !error; ++i)
        {
            unsigned char value = (unsigned char)i, reply = 0;
            error = one_byte(outbound[1], &value, 1);
            if (!error)
                error = one_byte(inbound[0], &reply, 0);
            if (!error && value != reply)
                error = EIO;
            checksum += reply;
        }
        if (!error)
            error = stamp(&ended);
        close(outbound[1]);
        close(inbound[0]);
        if (child > 0)
        {
            int status = 0;
            pid_t waited;
            do
                waited = waitpid(child, &status, 0);
            while (waited < 0 && errno == EINTR);
            if (!error && (waited != child || !WIFEXITED(status) || WEXITSTATUS(status)))
                error = EIO;
        }
        result("pipe_pingpong", 2, sample, ended >= began ? ended - began : 0, error ? 0 : count,
               error ? 0 : (uint64_t)count * 2, checksum, error);
        if (error)
            return;
    }
}

struct group
{
    unsigned ready, start;
};

struct worker
{
    struct group *group;
    uint64_t operations, checksum;
    unsigned identity;
    int syscalls, error;
};

static void *worker_main(void *context)
{
    struct worker *worker = context;
    __atomic_add_fetch(&worker->group->ready, 1, __ATOMIC_RELEASE);
    while (!__atomic_load_n(&worker->group->start, __ATOMIC_ACQUIRE))
        sched_yield();
    if (worker->syscalls)
        worker->error = pid_loop(worker->operations, &worker->checksum);
    else
        worker->checksum = cpu_loop(worker->operations, UINT32_C(0x12345678) + worker->identity);
    return NULL;
}

static void threads_case(unsigned count, int syscalls)
{
    const char *name = syscalls ? "threads_getpid_kernel" : "threads_cpu_u32";
    uint64_t operations = (uint64_t)(syscalls ? PID_OPERATIONS : CPU_OPERATIONS) * scale;
    for (unsigned sample = 0; sample <= repetitions; ++sample)
    {
        struct group group = {0};
        struct worker workers[4] = {0};
        pthread_t threads[4];
        unsigned created = 0;
        int error = 0;
        for (; created < count; ++created)
        {
            workers[created] = (struct worker){.group = &group,
                                               .operations = operations / count,
                                               .identity = created,
                                               .syscalls = syscalls};
            error = pthread_create(threads + created, NULL, worker_main, workers + created);
            if (error)
                break;
        }
        while (!error && __atomic_load_n(&group.ready, __ATOMIC_ACQUIRE) != count)
            sched_yield();
        uint64_t began = 0, ended = 0, checksum = 0;
        if (!error)
            error = stamp(&began);
        if (error)
            for (unsigned i = 0; i < created; ++i)
                workers[i].operations = 0;
        __atomic_store_n(&group.start, 1, __ATOMIC_RELEASE);
        for (unsigned i = 0; i < created; ++i)
        {
            int joined = pthread_join(threads[i], NULL);
            if (!error && (joined || workers[i].error))
                error = joined ? joined : workers[i].error;
            checksum += workers[i].checksum;
        }
        if (!error)
            error = stamp(&ended);
        result(name, count, sample, ended >= began ? ended - began : 0, error ? 0 : operations, 0,
               checksum, error);
        if (error)
            return;
    }
}

static unsigned argument(const char *text, unsigned maximum)
{
    char *end;
    unsigned long value = strtoul(text, &end, 10);
    return *text && !*end && value && value <= maximum ? (unsigned)value : 0;
}

static int benchmark(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i)
        if (!strcmp(argv[i], "--scale") && i + 1 < argc)
        {
            scale = argument(argv[++i], 64);
            if (!scale)
                return 2;
        }
        else if (!strcmp(argv[i], "--reps") && i + 1 < argc)
        {
            repetitions = argument(argv[++i], 9);
            if (!repetitions)
                return 2;
        }
        else if (!strcmp(argv[i], "--case") && i + 1 < argc)
            selection = argv[++i];
        else
            return 2;
    if (strcmp(selection, "all") && strcmp(selection, "cpu") && strcmp(selection, "memory") &&
        strcmp(selection, "syscall") && strcmp(selection, "pipe") &&
        strcmp(selection, "threads_cpu") && strcmp(selection, "threads_getpid"))
        return 2;
    struct timespec precision;
    if (clock_getres(CLOCK_MONOTONIC, &precision))
        return 1;
    resolution =
        (uint64_t)precision.tv_sec * UINT64_C(1000000000) + (unsigned long)precision.tv_nsec;
    if (!resolution)
        resolution = 1;
    printf("META,pointer_bits,%u\nMETA,clock_resolution_ns,%llu\n", (unsigned)(sizeof(void *) * 8),
           (unsigned long long)resolution);
    printf(
        "HEADER,case,workers,phase,sample,scale,elapsed_ns,operations,bytes,checksum,status,errno\n");
    fflush(stdout);
    if (selected("cpu"))
        cpu_case();
    if (selected("memory"))
    {
        memory_case(0);
        memory_case(1);
        pointer_case();
    }
    if (selected("syscall"))
        syscall_case();
    if (selected("pipe"))
        pipe_case();
    for (unsigned workers = 1; workers <= 4; workers *= 2)
    {
        if (selected("threads_cpu"))
            threads_case(workers, 0);
        if (selected("threads_getpid"))
            threads_case(workers, 1);
    }
    printf("BENCH_DONE,errors,%u,sink,%llu\n", failed, (unsigned long long)observed);
    fflush(stdout);
    return failed ? 1 : 0;
}

#ifdef LUME_BENCH_APP
int app_main(int argc, char **argv, char **environment)
{
    (void)environment;
    return benchmark(argc, argv);
}
#else
int main(int argc, char **argv)
{
    int status = benchmark(argc, argv);
#ifdef __linux__
    if (getpid() == 1)
    {
        fflush(stdout);
        sync();
        reboot(RB_POWER_OFF);
        for (;;)
            pause();
    }
#endif
    return status;
}
#endif
