/* SPDX-License-Identifier: BSD-3-Clause */
#include "lume/abi.h"
#include "lume/gpu.h"
#include "lume/syscall_native.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define WIDTH 384u
#define HEIGHT 96u
#define PITCH (WIDTH * 4u)
static unsigned char reference[PITCH * HEIGHT];
static unsigned char snapshot[PITCH * HEIGHT];

static void require(int value, const char *name)
{
    if (!value)
    {
        printf("AMD64_GPU_2D_FAIL %s\n", name);
        exit(1);
    }
}

static int call(unsigned action, struct lume_gpu_request *request)
{
    request->version = LUME_GPU_VERSION;
    request->bytes = sizeof(*request);
    return (int)lume_native_call(LUME_SYS_GPU, action, (uintptr_t)request, sizeof(*request));
}

int app_main(void)
{
    struct lume_gpu_request query = {.fd = -1};
    require(!call(LUME_GPU_QUERY, &query), "query");
    require(query.backend == LUME_GPU_BACKEND_QEMU2D &&
                (query.capabilities & LUME_GPU_COPY_OVERLAP),
            "backend");
    unsigned char *pixels;
    require(!posix_memalign((void **)&pixels, 4096, sizeof(reference)), "allocation");
    struct lume_gpu_request binding = {
        .fd = -1, .pixels = (uintptr_t)pixels, .width = WIDTH, .height = HEIGHT, .pitch = PITCH};
    require(!call(LUME_GPU_BIND, &binding), "bind");
    const struct lume_gpu_rect cases[][2] = {
        {{0, 0, 9, 0, WIDTH - 9, HEIGHT}, {0}},
        {{0, 0, 0, 7, WIDTH, HEIGHT - 7}, {0}},
        {{7, 9, 0, 0, WIDTH - 7, HEIGHT - 9}, {0}},
        {{5, 3, 18, 62, 260, 32}, {0}},
        {{0, 0, 0, HEIGHT / 2, WIDTH, HEIGHT / 2}, {0, HEIGHT / 2, 0, 0, WIDTH, HEIGHT / 2}},
    };
    for (unsigned repeat = 0; repeat < 8; ++repeat)
        for (unsigned test = 0; test < sizeof(cases) / sizeof(cases[0]); ++test)
        {
            for (unsigned i = 0; i < sizeof(reference); ++i)
                pixels[i] = (unsigned char)(i * 37 + i / 509 + repeat * 13);
            memcpy(reference, pixels, sizeof(reference));
            memcpy(snapshot, pixels, sizeof(snapshot));
            unsigned count = cases[test][1].width ? 2 : 1;
            for (unsigned i = 0; i < count; ++i)
            {
                const struct lume_gpu_rect *r = cases[test] + i;
                for (unsigned y = 0; y < r->height; ++y)
                    memcpy(reference + (r->target_y + y) * PITCH + r->target_x * 4,
                           snapshot + (r->source_y + y) * PITCH + r->source_x * 4, r->width * 4);
            }
            struct lume_gpu_request request = {
                .fd = binding.fd, .count = count, .rectangles = (uintptr_t)cases[test]};
            require(!call(LUME_GPU_COPY, &request), "copy");
            require(!memcmp(reference, pixels, sizeof(reference)), "bytes");
        }
    struct lume_gpu_rect invalid = {WIDTH, 0, 0, 0, 1, 1};
    struct lume_gpu_request request = {
        .fd = binding.fd, .count = 1, .rectangles = (uintptr_t)&invalid};
    require(call(LUME_GPU_COPY, &request) == -LUME_EINVAL, "bounds");
    require(!memcmp(reference, pixels, sizeof(reference)), "invalid copy unchanged");
    require(!close(binding.fd), "close");
    require(call(LUME_GPU_COPY, &request) == -LUME_EBADF, "closed descriptor");
    binding = (struct lume_gpu_request){
        .fd = -1, .pixels = (uintptr_t)pixels, .width = WIDTH, .height = HEIGHT, .pitch = PITCH};
    require(!call(LUME_GPU_BIND, &binding), "rebind");
    require(!close(binding.fd), "release");
    free(pixels);
    puts("AMD64_GPU_2D_PASS copies=40 overlap=snapshot bounds=pass lifecycle=pass");
    return 0;
}
