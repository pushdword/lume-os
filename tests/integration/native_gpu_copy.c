#define _POSIX_C_SOURCE 200809L
#include "lume/abi.h"
#include "lume/gpu.h"
#include "lume/syscall_native.h"
#include "perf.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static int gpu(unsigned action, struct lume_gpu_request *request)
{
    request->version = LUME_GPU_VERSION;
    request->bytes = sizeof(*request);
    return (int)lume_native_call(LUME_SYS_GPU, action, (uintptr_t)request, sizeof(*request));
}

static uint64_t milliseconds(void)
{
    return (uint64_t)lume_native_call(LUME_SYS_CLOCK, 0, 0, 0);
}

static int copy_gpu(struct lume_gpu_request *request)
{
    uint64_t end = milliseconds() + 500;
    int result;
    do
    {
        result = gpu(LUME_GPU_COPY, request);
        if (result != -LUME_EAGAIN)
            return result;
        struct timespec delay = {.tv_nsec = 1000000};
        nanosleep(&delay, NULL);
    } while (milliseconds() < end);
    return result;
}

static int snapshot(struct lume_perf_snapshot *out)
{
    return (int)lume_native_call(LUME_SYS_TELEMETRY, (uintptr_t)out, sizeof(*out),
                                 LUME_PERF_VERSION);
}

static uint32_t cpu_ticks(const struct lume_perf_snapshot *out)
{
    uint32_t ticks = 0;
    for (unsigned i = 0; i < out->core_count; ++i)
        ticks += out->cores[i].ticks[LUME_PERF_USER] + out->cores[i].ticks[LUME_PERF_KERNEL] +
                 out->cores[i].ticks[LUME_PERF_LOCKWAIT];
    return ticks;
}

int main(void)
{
    struct lume_gpu_request query = {.fd = -1};
    int result = gpu(LUME_GPU_QUERY, &query);
    if (result)
    {
        printf("GPU_COPY unavailable=%d\n", result);
        return 1;
    }
    uint32_t *pixels, *expected = malloc(1024u * 1024u);
    if (!expected || posix_memalign((void **)&pixels, 4096, 1024u * 1024u))
        return 2;
    const uint32_t special[] = {0,          UINT32_MAX, 0x7f800000, 0xff800000,
                                0x7fc12345, 0x7fa12345, 1,          0x80000000};
    for (unsigned i = 0; i < 512u * 512u; ++i)
        pixels[i] = expected[i] = i % 16 < 8 ? special[i % 8] : i * 65537u;
    struct lume_gpu_request binding = {
        .fd = -1, .pixels = (uintptr_t)pixels, .width = 512, .height = 512, .pitch = 2048};
    result = gpu(LUME_GPU_BIND, &binding);
    if (result)
    {
        printf("GPU_COPY bind=%d\n", result);
        return 3;
    }
    struct lume_gpu_rect rectangle = {0, 0, 0, 32, 512, 448};
    struct lume_gpu_request request = {
        .fd = binding.fd, .count = 1, .rectangles = (uintptr_t)&rectangle};
    memmove(expected + 32 * 512, expected, 448u * 2048u);
    result = copy_gpu(&request);
    if (!result && memcmp(pixels, expected, 1024u * 1024u))
        result = -LUME_EIO;
    if (result)
    {
        printf("GPU_COPY integrity=%d\n", result);
        close(binding.fd);
        return 4;
    }
    struct lume_perf_snapshot before, after;
    if (snapshot(&before))
        return 5;
    uint64_t start = milliseconds();
    for (unsigned i = 0; i < 32; ++i)
    {
        memmove(expected + 32 * 512, expected, 448u * 2048u);
        __asm__ volatile("" ::: "memory");
    }
    uint64_t cpu_ms = milliseconds() - start;
    if (snapshot(&after))
        return 5;
    uint32_t cpu = cpu_ticks(&after) - cpu_ticks(&before);
    if (snapshot(&before))
        return 5;
    start = milliseconds();
    for (unsigned i = 0; i < 32; ++i)
    {
        result = copy_gpu(&request);
        if (result)
            break;
    }
    uint64_t gpu_ms = milliseconds() - start;
    if (snapshot(&after))
        return 5;
    uint32_t accelerated = cpu_ticks(&after) - cpu_ticks(&before);
    int equal = !memcmp(pixels, expected, 1024u * 1024u);
    close(binding.fd);
    free(pixels);
    free(expected);
    printf("GPU_COPY result=%d equal=%d cpu_ms=%llu gpu_ms=%llu cpu_ticks=%u gpu_ticks=%u hz=%u\n",
           result, equal, (unsigned long long)cpu_ms, (unsigned long long)gpu_ms, cpu, accelerated,
           after.clock_hz);
    return result || !equal ? 6 : 0;
}
