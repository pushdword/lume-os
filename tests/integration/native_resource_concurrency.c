#define _POSIX_C_SOURCE 200809L
#include "lume/abi.h"
#include "lume/graphics.h"
#include "lume/net_commands.h"
#include "lume/syscall_native.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include "native_resource_gate.h"

static atomic_int ready, started, done;
static struct lume_net_request network;
static uint64_t net_start, net_end, beat_gap, beat_io, beat_samples;
static pthread_mutex_t measurement_lock = PTHREAD_MUTEX_INITIALIZER;
static uint64_t measure_start, measure_end, beat_previous;
static bool measure_open, measure_closed;
static int64_t net_result;
static int endpoints[2], beat_error;

static uint64_t now(void)
{
    return (uint64_t)lume_native_call(LUME_SYS_CLOCK, 0, 0, 0);
}

static void pause_ms(unsigned ms)
{
    struct timespec delay = {.tv_sec = ms / 1000, .tv_nsec = (long)(ms % 1000) * 1000000};
    nanosleep(&delay, NULL);
}

static void *heartbeat(void *unused)
{
    (void)unused;
    atomic_store(&ready, 1);
    while (!atomic_load(&done))
    {
        char sent = 'r', received = 0;
        uint64_t before = now();
        if (write(endpoints[0], &sent, 1) != 1 || read(endpoints[1], &received, 1) != 1 ||
            received != sent)
        {
            beat_error = 1;
            break;
        }
        uint64_t after = now();
        pthread_mutex_lock(&measurement_lock);
        /* Completion timestamps define membership even when publication follows window close. */
        if (measure_open && after >= measure_start && (!measure_closed || after <= measure_end))
        {
            if (after - before > beat_io)
                beat_io = after - before;
            if (after - beat_previous > beat_gap)
                beat_gap = after - beat_previous;
            beat_previous = after;
            ++beat_samples;
        }
        pthread_mutex_unlock(&measurement_lock);
        pause_ms(1);
    }
    return NULL;
}

static void *network_wait(void *unused)
{
    (void)unused;
    pthread_mutex_lock(&measurement_lock);
    net_start = measure_start = beat_previous = now();
    measure_open = true;
    pthread_mutex_unlock(&measurement_lock);
    atomic_store(&started, 1);
    net_result = lume_native_call(LUME_SYS_NET, LUME_NET_TCP, (uintptr_t)&network, sizeof(network));
    net_end = now();
    return NULL;
}

int main(int argc, char **argv)
{
    struct resource_limits limits;
    if (argc != 7 || strlen(argv[1]) >= sizeof(network.argument) ||
        !positive_budget(argv[3], &limits.query_ms) || !positive_budget(argv[4], &limits.read_ms) ||
        !positive_budget(argv[5], &limits.io_ms) || !positive_budget(argv[6], &limits.gap_ms))
    {
        fprintf(stderr, "Usage: %s NETWORK FILE MAX_QUERY_MS MAX_READ_MS MAX_IO_MS MAX_GAP_MS\n",
                argv[0]);
        return 2;
    }
    memcpy(network.argument, argv[1], strlen(argv[1]) + 1);
    int fd = open(argv[2], O_RDONLY);
    struct lume_stat stat;
    if (fd < 0 || lume_native_call(LUME_SYS_STAT, fd, (uintptr_t)&stat, 0) < 0 || !stat.size ||
        socketpair(AF_UNIX, SOCK_STREAM, 0, endpoints))
        return 3;
    if (stat.size > 2097152)
        stat.size = 2097152;
    unsigned char *buffer = malloc(65536);
    if (!buffer)
        return 4;
    pthread_t heart, net;
    if (pthread_create(&heart, NULL, heartbeat, NULL))
        return 5;
    while (!atomic_load(&ready))
        pause_ms(1);
    if (pthread_create(&net, NULL, network_wait, NULL))
        return 6;
    while (!atomic_load(&started))
        pause_ms(1);
    pause_ms(100);
    struct lume_graphics_request display = {
        .version = LUME_GRAPHICS_VERSION, .bytes = sizeof(display), .fd = -1};
    uint64_t query_start = now();
    int64_t query = lume_native_call(LUME_SYS_GRAPHICS, LUME_GRAPHICS_QUERY, (uintptr_t)&display,
                                     sizeof(display));
    uint64_t query_end = now();
    uint64_t storage_start = now(), bytes = 0;
    unsigned sample = 0;
    while (bytes < stat.size)
    {
        unsigned count = stat.size - bytes > 65536 ? 65536 : (unsigned)(stat.size - bytes);
        int64_t n = lume_native_call(LUME_SYS_READ, fd, (uintptr_t)buffer, count);
        if (n <= 0 || (uint64_t)n > count)
            return 7;
        bytes += (uint64_t)n;
        sample = sample * 33u + buffer[0] + buffer[n - 1];
    }
    uint64_t storage_end = now();
    if (pthread_join(net, NULL))
        return 8;
    pthread_mutex_lock(&measurement_lock);
    measure_end = now();
    measure_closed = true;
    atomic_store(&done, 1);
    pthread_mutex_unlock(&measurement_lock);
    if (pthread_join(heart, NULL))
        return 9;
    pthread_mutex_lock(&measurement_lock);
    if (measure_end - beat_previous > beat_gap)
        beat_gap = measure_end - beat_previous;
    pthread_mutex_unlock(&measurement_lock);
    close(fd);
    close(endpoints[0]);
    close(endpoints[1]);
    free(buffer);
    struct resource_measurements measured = {
        .net_result = net_result,
        .query_result = query,
        .bytes = bytes,
        .query_ms = query_end - query_start,
        .read_ms = storage_end - storage_start,
        .io_ms = beat_io,
        .gap_ms = beat_gap,
        .samples = beat_samples,
        .display_overlap = query_start >= net_start && query_end < net_end,
        .storage_overlap = storage_start >= net_start && storage_end < net_end,
        .error = beat_error};
    printf("RESOURCE_IO net_result=%lld query_result=%lld bytes=%llu sample=%u net_ms=%llu "
           "query_ms=%llu read_ms=%llu display_overlap=%u storage_overlap=%u io_ms=%llu "
           "gap_ms=%llu samples=%llu window_ms=%llu error=%d max_query_ms=%llu max_read_ms=%llu "
           "max_io_ms=%llu max_gap_ms=%llu\n",
           (long long)net_result, (long long)query, (unsigned long long)bytes, sample,
           (unsigned long long)(net_end - net_start), (unsigned long long)measured.query_ms,
           (unsigned long long)measured.read_ms, measured.display_overlap, measured.storage_overlap,
           (unsigned long long)beat_io, (unsigned long long)beat_gap,
           (unsigned long long)beat_samples, (unsigned long long)(measure_end - measure_start),
           beat_error, (unsigned long long)limits.query_ms, (unsigned long long)limits.read_ms,
           (unsigned long long)limits.io_ms, (unsigned long long)limits.gap_ms);
    return resource_passed(&measured, &limits) ? 0 : 10;
}
