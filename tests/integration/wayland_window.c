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
#include "../../ports/wayland/runtime.h"

#define WIDTH 320
#define HEIGHT 240

struct state
{
    struct wl_display *display;
    struct wl_compositor *compositor;
    struct wl_shm *shm;
    struct xdg_wm_base *wm;
    struct wl_surface *surface;
    struct wl_buffer *buffers[2];
    uint32_t *pixels;
    unsigned frames, limit, next;
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
    struct state state = {.limit = 60};
    if (argc == 2)
    {
        char *end;
        unsigned long limit = strtoul(argv[1], &end, 10);
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
    size_t bytes = 2 * WIDTH * HEIGHT * sizeof(uint32_t);
    int fd = memfd_create("wayland-window", MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (fd < 0 || ftruncate(fd, bytes))
        goto fail;
    state.pixels = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (state.pixels == MAP_FAILED)
        goto fail;
    struct wl_shm_pool *pool = wl_shm_create_pool(state.shm, fd, bytes);
    for (unsigned i = 0; i < 2; ++i)
    {
        state.buffers[i] = wl_shm_pool_create_buffer(pool, i * WIDTH * HEIGHT * 4, WIDTH, HEIGHT,
                                                     WIDTH * 4, WL_SHM_FORMAT_XRGB8888);
        wl_buffer_add_listener(state.buffers[i], &buffer_listener, &state);
    }
    wl_shm_pool_destroy(pool);
    close(fd);
    state.surface = wl_compositor_create_surface(state.compositor);
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
    uint64_t started = now();
    draw(&state);
    while (state.frames < state.limit && !state.closed)
        if (wl_display_dispatch(state.display) < 0)
            goto fail;
    uint64_t elapsed = now() - started;
    if (state.closed)
        goto fail;
    xdg_toplevel_destroy(toplevel);
    xdg_surface_destroy(surface);
    wl_surface_destroy(state.surface);
    for (unsigned i = 0; i < 2; ++i)
        wl_buffer_destroy(state.buffers[i]);
    xdg_wm_base_destroy(state.wm);
    wl_shm_destroy(state.shm);
    wl_compositor_destroy(state.compositor);
    wl_registry_destroy(registry);
    wl_display_flush(state.display);
    wl_display_disconnect(state.display);
    munmap(state.pixels, bytes);
    printf("WAYLAND_WINDOW_PASS frames=%u elapsed_ns=%llu\n", state.frames,
           (unsigned long long)elapsed);
    return 0;
fail:
    fprintf(stderr, "WAYLAND_WINDOW_FAIL errno=%d\n", errno);
    return 1;
}
