/* SPDX-License-Identifier: BSD-3-Clause */
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include "xdg-shell-client-protocol.h"
#include "../../ports/wayland/runtime.h"

struct state
{
    struct wl_compositor *compositor;
    struct xdg_wm_base *wm;
    unsigned dmabuf_version;
    bool configured, presented, closed;
};

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
    else if (!strcmp(interface, "xdg_wm_base"))
    {
        state->wm = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(state->wm, &wm_listener, state);
    }
    else if (!strcmp(interface, "zwp_linux_dmabuf_v1"))
        state->dmabuf_version = version;
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
    xdg_surface_ack_configure(surface, serial);
    ((struct state *)data)->configured = true;
}

static const struct xdg_surface_listener surface_listener = {.configure = configure};

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

static const struct xdg_toplevel_listener toplevel_listener = {toplevel_configure, toplevel_close};

static void frame_done(void *data, struct wl_callback *callback, uint32_t time)
{
    (void)time;
    wl_callback_destroy(callback);
    ((struct state *)data)->presented = true;
}

static const struct wl_callback_listener frame_listener = {.done = frame_done};

int main(void)
{
    struct state state = {0};
    const char *stage = "runtime";
    if (lume_wayland_runtime())
        goto fail;
    struct wl_display *display = wl_display_connect(NULL);
    if (!display)
        goto fail;
    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, &state);
    stage = "globals";
    if (wl_display_roundtrip(display) < 0 || !state.compositor || !state.wm ||
        state.dmabuf_version < 4)
        goto fail;
    struct wl_surface *surface = wl_compositor_create_surface(state.compositor);
    struct xdg_surface *xdg = xdg_wm_base_get_xdg_surface(state.wm, surface);
    xdg_surface_add_listener(xdg, &surface_listener, &state);
    struct xdg_toplevel *toplevel = xdg_surface_get_toplevel(xdg);
    xdg_toplevel_set_title(toplevel, "Wayland EGL check");
    xdg_toplevel_add_listener(toplevel, &toplevel_listener, &state);
    wl_surface_commit(surface);
    while (!state.configured && !state.closed)
        if (wl_display_dispatch(display) < 0)
            goto fail;
    stage = "egl-display";
    EGLDisplay egl = eglGetPlatformDisplay(EGL_PLATFORM_WAYLAND_KHR, display, NULL);
    EGLint major, minor;
    if (state.closed || egl == EGL_NO_DISPLAY || !eglInitialize(egl, &major, &minor) ||
        !eglBindAPI(EGL_OPENGL_ES_API))
        goto fail;
    const EGLint attributes[] = {EGL_SURFACE_TYPE,
                                 EGL_WINDOW_BIT,
                                 EGL_RENDERABLE_TYPE,
                                 EGL_OPENGL_ES2_BIT,
                                 EGL_RED_SIZE,
                                 8,
                                 EGL_GREEN_SIZE,
                                 8,
                                 EGL_BLUE_SIZE,
                                 8,
                                 EGL_ALPHA_SIZE,
                                 8,
                                 EGL_NONE};
    EGLConfig config;
    EGLint count;
    stage = "egl-config";
    if (!eglChooseConfig(egl, attributes, &config, 1, &count) || count != 1)
        goto fail;
    struct wl_egl_window *window = wl_egl_window_create(surface, 320, 240);
    if (!window)
        goto fail;
    const EGLint context_attributes[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    EGLContext context = eglCreateContext(egl, config, EGL_NO_CONTEXT, context_attributes);
    EGLSurface target = eglCreateWindowSurface(egl, config, (EGLNativeWindowType)window, NULL);
    stage = "egl-context";
    if (context == EGL_NO_CONTEXT || target == EGL_NO_SURFACE ||
        !eglMakeCurrent(egl, target, target, context))
        goto fail;
    const char *renderer = (const char *)glGetString(GL_RENDERER);
    if (!renderer || strstr(renderer, "llvmpipe") || strstr(renderer, "softpipe") ||
        strstr(renderer, "SwiftShader"))
        goto fail;
    glViewport(0, 0, 320, 240);
    glClearColor(0.125f, 0.5f, 0.75f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    uint8_t pixel[4] = {0};
    glReadPixels(160, 120, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    stage = "pixel";
    if (glGetError() != GL_NO_ERROR || pixel[0] < 31 || pixel[0] > 33 || pixel[1] < 127 ||
        pixel[1] > 129 || pixel[2] < 190 || pixel[2] > 192 || pixel[3] != 255)
        goto fail;
    struct wl_callback *frame = wl_surface_frame(surface);
    wl_callback_add_listener(frame, &frame_listener, &state);
    stage = "swap";
    if (!eglSwapBuffers(egl, target))
        goto fail;
    stage = "frame";
    while (!state.presented && !state.closed)
        if (wl_display_dispatch(display) < 0)
            goto fail;
    if (!state.presented)
        goto fail;
    printf("WAYLAND_EGL_PASS renderer=%s dmabuf_version=%u rgba=%u,%u,%u,%u\n", renderer,
           state.dmabuf_version, pixel[0], pixel[1], pixel[2], pixel[3]);
    eglMakeCurrent(egl, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(egl, target);
    eglDestroyContext(egl, context);
    wl_egl_window_destroy(window);
    eglTerminate(egl);
    xdg_toplevel_destroy(toplevel);
    xdg_surface_destroy(xdg);
    wl_surface_destroy(surface);
    xdg_wm_base_destroy(state.wm);
    wl_compositor_destroy(state.compositor);
    wl_registry_destroy(registry);
    wl_display_flush(display);
    wl_display_disconnect(display);
    return 0;
fail:
    fprintf(stderr, "WAYLAND_EGL_FAIL stage=%s egl=%#x errno=%d\n", stage, eglGetError(), errno);
    return 1;
}
