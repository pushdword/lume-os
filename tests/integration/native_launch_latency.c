#define _POSIX_C_SOURCE 200809L
#define _POSIX_MONOTONIC_CLOCK 200809L
#include <pthread.h>
#include <spawn.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "lume/abi.h"
#include "lume/syscall_native.h"

static atomic_int ready, done;
static int endpoints[2], io_error;
static int raw_read;
static uint64_t maximum_gap, maximum_io, maximum_write, maximum_read, samples;

static uint64_t now(void)
{
    struct timespec value;
    if (clock_gettime(CLOCK_MONOTONIC, &value))
        return 0;
    return (uint64_t)value.tv_sec * UINT64_C(1000000000) + value.tv_nsec;
}

static void *heartbeat(void *unused)
{
    (void)unused;
    uint64_t previous = now();
    atomic_store_explicit(&ready, 1, memory_order_release);
    while (!atomic_load_explicit(&done, memory_order_acquire))
    {
        char sent = 'a', received = 0;
        uint64_t before = now();
        int written = write(endpoints[0], &sent, 1);
        uint64_t between = now();
        int received_bytes =
            raw_read ? (int)lume_native_call(LUME_SYS_READ, endpoints[1], (uintptr_t)&received, 1)
                     : read(endpoints[1], &received, 1);
        if (written != 1 || received_bytes != 1 || received != sent)
        {
            io_error = 1;
            break;
        }
        uint64_t after = now();
        if (between - before > maximum_write)
            maximum_write = between - before;
        if (after - between > maximum_read)
            maximum_read = after - between;
        if (after - before > maximum_io)
            maximum_io = after - before;
        if (after - previous > maximum_gap)
            maximum_gap = after - previous;
        previous = after;
        ++samples;
        struct timespec delay = {.tv_nsec = 1000000};
        nanosleep(&delay, NULL);
    }
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 2 && argc != 3)
        return 2;
    raw_read = argc == 3;
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, endpoints))
        return 3;
    pthread_t thread;
    if (pthread_create(&thread, NULL, heartbeat, NULL))
        return 4;
    while (!atomic_load_explicit(&ready, memory_order_acquire))
    {
        struct timespec delay = {.tv_nsec = 1000000};
        nanosleep(&delay, NULL);
    }
    char *arguments[] = {argv[1], "--version", NULL};
    char *environment[] = {"PATH=/usr/bin:/bin:/data/bin", "HOME=/data/home", "LANG=C", NULL};
    uint64_t before = now();
    pid_t child;
    int result = posix_spawn(&child, argv[1], NULL, NULL, arguments, environment);
    uint64_t spawned = now();
    int status = -1;
    if (!result && waitpid(child, &status, 0) != child)
        result = 5;
    uint64_t elapsed = now() - before;
    atomic_store_explicit(&done, 1, memory_order_release);
    if (pthread_join(thread, NULL))
        return 6;
    close(endpoints[0]);
    close(endpoints[1]);
    printf(
        "LAUNCH_LATENCY result=%d status=%d elapsed_ns=%llu gap_ns=%llu io_ns=%llu samples=%llu error=%d spawn_ns=%llu write_ns=%llu read_ns=%llu\n",
        result, status, (unsigned long long)elapsed, (unsigned long long)maximum_gap,
        (unsigned long long)maximum_io, (unsigned long long)samples, io_error,
        (unsigned long long)(spawned - before), (unsigned long long)maximum_write,
        (unsigned long long)maximum_read);
    return result || status || io_error || !samples ? 1 : 0;
}
