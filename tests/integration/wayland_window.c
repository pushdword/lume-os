/* SPDX-License-Identifier: BSD-3-Clause */
#define _GNU_SOURCE
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include "viewporter-client-protocol.h"
#include "../../ports/wayland/runtime.h"
#ifdef __lume__
#include "lume/abi.h"
#include "lume/syscall_native.h"
#include "perf.h"
#endif

#define WIDTH 320
#define HEIGHT 240

struct state
{
    struct wl_display *display;
    struct wl_compositor *compositor;
    struct wl_shm *shm;
    struct xdg_wm_base *wm;
    struct wp_viewporter *viewporter;
    struct wp_viewport *viewport;
    struct wl_surface *surface;
    struct wl_buffer *buffers[2];
    uint32_t *pixels;
    unsigned frames, limit, next;
    unsigned width, height, buffer_height;
    bool scroll;
    bool busy[2], configured, closed;
};

static uint64_t now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000u + ts.tv_nsec;
}

static void ping(void *data, struct xdg_wm_base *wm, uint32_t serial)
{
    (void)data;
    xdg_wm_base_pong(wm, serial);
}
static const struct xdg_wm_base_listener wm_listener = {.ping = ping};

static void global(void *data, struct wl_registry *registry, uint32_t name, const char *interface,
                   uint32_t version)
{
    struct state *state = data;
    if (!strcmp(interface, "wl_compositor"))
        state->compositor =
            wl_registry_bind(registry, name, &wl_compositor_interface, version < 4 ? version : 4);
    else if (!strcmp(interface, "wl_shm"))
        state->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
    else if (!strcmp(interface, "wp_viewporter"))
        state->viewporter = wl_registry_bind(registry, name, &wp_viewporter_interface, 1);
    else if (!strcmp(interface, "xdg_wm_base"))
    {
        state->wm = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(state->wm, &wm_listener, state);
    }
}

static void global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
    (void)data;
    (void)registry;
    (void)name;
}
static const struct wl_registry_listener registry_listener = {global, global_remove};

static void configure(void *data, struct xdg_surface *surface, uint32_t serial)
{
    struct state *state = data;
    xdg_surface_ack_configure(surface, serial);
    state->configured = true;
}
static const struct xdg_surface_listener surface_listener = {configure};

static void toplevel_configure(void *data, struct xdg_toplevel *toplevel, int32_t width,
                               int32_t height, struct wl_array *states)
{
    (void)data;
    (void)toplevel;
    (void)width;
    (void)height;
    (void)states;
}

static void toplevel_close(void *data, struct xdg_toplevel *toplevel)
{
    (void)toplevel;
    ((struct state *)data)->closed = true;
}
static const struct xdg_toplevel_listener toplevel_listener = {.configure = toplevel_configure,
                                                               .close = toplevel_close};

static void released(void *data, struct wl_buffer *buffer)
{
    struct state *state = data;
    for (unsigned i = 0; i < 2; ++i)
        if (state->buffers[i] == buffer)
            state->busy[i] = false;
}
static const struct wl_buffer_listener buffer_listener = {released};

static void draw(struct state *state);

static void frame_done(void *data, struct wl_callback *callback, uint32_t time)
{
    (void)time;
    struct state *state = data;
    wl_callback_destroy(callback);
    ++state->frames;
    if (state->frames < state->limit)
        draw(state);
}
static const struct wl_callback_listener frame_listener = {frame_done};

static void draw(struct state *state)
{
    if (state->scroll)
    {
        wl_surface_attach(state->surface, state->buffers[0], 0, 0);
        wp_viewport_set_source(state->viewport, 0,
                               wl_fixed_from_int((state->frames * 3) % state->height),
                               wl_fixed_from_int(state->width), wl_fixed_from_int(state->height));
        struct wl_callback *callback = wl_surface_frame(state->surface);
        wl_callback_add_listener(callback, &frame_listener, state);
        wl_surface_damage(state->surface, 0, 0, state->width, state->height);
        wl_surface_commit(state->surface);
        return;
    }
    unsigned index = state->next++ % 2;
    if (state->busy[index])
    {
        fprintf(stderr, "WAYLAND_WINDOW_FAIL buffer still owned by compositor\n");
        state->closed = true;
        return;
    }
    uint32_t *pixels = state->pixels + index * WIDTH * HEIGHT;
    unsigned phase = state->frames * 3;
    for (unsigned y = 0; y < HEIGHT; ++y)
        for (unsigned x = 0; x < WIDTH; ++x)
            pixels[y * WIDTH + x] =
                0xff000000u | (((x + phase) & 255) << 16) | (((y + phase) & 255) << 8) | 0x40;
    struct wl_callback *callback = wl_surface_frame(state->surface);
    wl_callback_add_listener(callback, &frame_listener, state);
    state->busy[index] = true;
    wl_surface_attach(state->surface, state->buffers[index], 0, 0);
    wl_surface_damage(state->surface, 0, 0, WIDTH, HEIGHT);
    wl_surface_commit(state->surface);
}

int main(int argc, char **argv)
{
    struct state state = {.limit = 60, .width = WIDTH, .height = HEIGHT, .buffer_height = HEIGHT};
    bool hold = false;
    for (int arg = 1; arg < argc; ++arg)
    {
        if (!strcmp(argv[arg], "--scroll"))
        {
            state.scroll = true;
            state.width = 800;
            state.height = 600;
            state.buffer_height = 1200;
            continue;
        }
        if (!strcmp(argv[arg], "--hold"))
        {
            hold = true;
            continue;
        }
        char *end;
        unsigned long limit = strtoul(argv[arg], &end, 10);
        if (*end || !limit || limit > 3600)
            return 2;
        state.limit = limit;
    }
    if (lume_wayland_runtime())
        goto fail;
    state.display = wl_display_connect(NULL);
    if (!state.display)
        goto fail;
    struct wl_registry *registry = wl_display_get_registry(state.display);
    wl_registry_add_listener(registry, &registry_listener, &state);
    if (wl_display_roundtrip(state.display) < 0 || !state.compositor || !state.shm || !state.wm)
        goto fail;
    if (state.scroll && !state.viewporter)
        goto fail;
    size_t bytes = 2 * state.width * state.buffer_height * sizeof(uint32_t);
    int fd = memfd_create("wayland-window", MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (fd < 0 || ftruncate(fd, bytes))
        goto fail;
    state.pixels = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (state.pixels == MAP_FAILED)
        goto fail;
    if (state.scroll)
        for (unsigned y = 0; y < state.buffer_height; ++y)
            for (unsigned x = 0; x < state.width; ++x)
                state.pixels[y * state.width + x] =
                    0xff000000u | ((x & 255) << 16) | ((y & 255) << 8) | 0x40;
    struct wl_shm_pool *pool = wl_shm_create_pool(state.shm, fd, bytes);
    for (unsigned i = 0; i < 2; ++i)
    {
        state.buffers[i] =
            wl_shm_pool_create_buffer(pool, i * state.width * state.buffer_height * 4, state.width,
                                      state.buffer_height, state.width * 4, WL_SHM_FORMAT_XRGB8888);
        wl_buffer_add_listener(state.buffers[i], &buffer_listener, &state);
    }
    wl_shm_pool_destroy(pool);
    close(fd);
    state.surface = wl_compositor_create_surface(state.compositor);
    if (state.scroll)
    {
        state.viewport = wp_viewporter_get_viewport(state.viewporter, state.surface);
        wp_viewport_set_destination(state.viewport, state.width, state.height);
    }
    struct wl_region *opaque = wl_compositor_create_region(state.compositor);
    wl_region_add(opaque, 0, 0, state.width, state.height);
    wl_surface_set_opaque_region(state.surface, opaque);
    wl_region_destroy(opaque);
    struct xdg_surface *surface = xdg_wm_base_get_xdg_surface(state.wm, state.surface);
    xdg_surface_add_listener(surface, &surface_listener, &state);
    struct xdg_toplevel *toplevel = xdg_surface_get_toplevel(surface);
    xdg_toplevel_set_title(toplevel, "Wayland animation");
    xdg_toplevel_set_app_id(toplevel, "lume.wayland.animation");
    xdg_toplevel_add_listener(toplevel, &toplevel_listener, &state);
    wl_surface_commit(state.surface);
    while (!state.configured)
        if (wl_display_dispatch(state.display) < 0)
            goto fail;
#ifdef __lume__
    struct lume_perf_snapshot before, after;
    if (lume_native_call(LUME_SYS_TELEMETRY, (uintptr_t)&before, sizeof(before), LUME_PERF_VERSION))
        goto fail;
#endif
    uint64_t started = now();
    draw(&state);
    while (state.frames < state.limit && !state.closed)
        if (wl_display_dispatch(state.display) < 0)
            goto fail;
    uint64_t elapsed = now() - started;
    if (state.closed)
        goto fail;
#ifdef __lume__
    if (lume_native_call(LUME_SYS_TELEMETRY, (uintptr_t)&after, sizeof(after), LUME_PERF_VERSION))
        goto fail;
    uint64_t ticks[LUME_PERF_MODES] = {0};
    for (unsigned cpu = 0; cpu < after.core_count; ++cpu)
        for (unsigned mode = 0; mode < LUME_PERF_MODES; ++mode)
            ticks[mode] += (uint32_t)(after.cores[cpu].ticks[mode] - before.cores[cpu].ticks[mode]);
    printf("WAYLAND_WINDOW_CPU hz=%u user=%llu kernel=%llu idle=%llu lock=%llu\n", after.clock_hz,
           (unsigned long long)ticks[LUME_PERF_USER], (unsigned long long)ticks[LUME_PERF_KERNEL],
           (unsigned long long)ticks[LUME_PERF_IDLE],
           (unsigned long long)ticks[LUME_PERF_LOCKWAIT]);
#endif
    printf("WAYLAND_WINDOW_PASS frames=%u elapsed_ns=%llu scroll=%u\n", state.frames,
           (unsigned long long)elapsed, state.scroll);
    fflush(stdout);
    if (hold)
        usleep(1500000);
    if (state.viewport)
        wp_viewport_destroy(state.viewport);
    xdg_toplevel_destroy(toplevel);
    xdg_surface_destroy(surface);
    wl_surface_destroy(state.surface);
    for (unsigned i = 0; i < 2; ++i)
        wl_buffer_destroy(state.buffers[i]);
    xdg_wm_base_destroy(state.wm);
    if (state.viewporter)
        wp_viewporter_destroy(state.viewporter);
    wl_shm_destroy(state.shm);
    wl_compositor_destroy(state.compositor);
    wl_registry_destroy(registry);
    wl_display_flush(state.display);
    wl_display_disconnect(state.display);
    munmap(state.pixels, bytes);
    return 0;
fail:
    fprintf(stderr, "WAYLAND_WINDOW_FAIL errno=%d\n", errno);
    return 1;
}
