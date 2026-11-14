/* SPDX-License-Identifier: BSD-3-Clause */
#include "config.h"
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <libweston/libweston.h>
#include <libweston/desktop.h>
#include "backend.h"
#include "libweston-internal.h"
#include "pixman-renderer.h"
#include "pixel-formats.h"
#include "presentation-time-server-protocol.h"
#include "shared/weston-drm-fourcc.h"
#include "lume/abi.h"
#include "lume/graphics.h"
#include "lume/syscall_native.h"
#include "../wayland/runtime.h"
#include "shell.h"
#include "gpu.h"

struct lume_compositor
{
    struct wl_display *display;
    struct weston_compositor *compositor;
    struct weston_backend backend;
    struct weston_head head;
    struct weston_output output;
    struct weston_mode mode;
    struct weston_seat seat;
    struct weston_layer layer, background_layer;
    struct weston_surface *background;
    struct weston_buffer_reference *background_buffer;
    struct weston_view *background_view;
    struct weston_desktop *desktop;
    struct weston_renderbuffer *renderbuffer;
    struct lume_weston_gpu *gpu;
    struct wl_event_source *finish_timer, *input_source;
    uint32_t *pixels;
    uint32_t width, height, pitch, buttons;
    unsigned windows;
    int fd, exit_code;
    bool held[256];
    uint64_t frames, damaged_pixels, render_ns, present_ns;
};

static uint64_t monotonic_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000u + ts.tv_nsec;
}

static int graphics(unsigned action, struct lume_graphics_request *request)
{
    request->version = LUME_GRAPHICS_VERSION;
    request->bytes = sizeof(*request);
    return (int)lume_native_call(LUME_SYS_GRAPHICS, action, (uintptr_t)request, sizeof(*request));
}

static int log_message(const char *format, va_list args)
{
    return vfprintf(stderr, format, args);
}

static int finish_frame(void *data)
{
    struct lume_compositor *lume = data;
    weston_output_finish_frame_from_timer(&lume->output);
    return 0;
}

static int start_repaint(struct weston_output *output)
{
    struct timespec now;
    weston_compositor_read_presentation_clock(output->compositor, &now);
    weston_output_finish_frame(output, &now, WP_PRESENTATION_FEEDBACK_INVALID);
    return 0;
}

static int repaint(struct weston_output *output)
{
    struct lume_compositor *lume = output->compositor->user_data;
    pixman_region32_t damage;
    pixman_region32_init(&damage);
    weston_output_flush_damage_for_primary_plane(output, &damage);
    uint64_t started = monotonic_ns();
    if (lume->gpu)
        lume->renderbuffer = lume_weston_gpu_next(lume->gpu);
    output->compositor->renderer->repaint_output(output, &damage, lume->renderbuffer);
#ifdef LUME_WESTON_RENDER_CHECK
    extern void lume_weston_render_check(struct weston_output *, uint64_t);
    lume_weston_render_check(output, lume->frames);
#endif
    lume->render_ns += monotonic_ns() - started;
    int count;
    pixman_box32_t *boxes = pixman_region32_rectangles(&damage, &count);
    started = monotonic_ns();
    for (int i = 0; i < count; ++i)
    {
        pixman_box32_t box = boxes[i];
        if (box.x1 < 0)
            box.x1 = 0;
        if (box.y1 < 0)
            box.y1 = 0;
        if (box.x2 > (int)lume->width)
            box.x2 = lume->width;
        if (box.y2 > (int)lume->height)
            box.y2 = lume->height;
        if (box.x2 <= box.x1 || box.y2 <= box.y1)
            continue;
        uint32_t width = box.x2 - box.x1, height = box.y2 - box.y1;
        lume->damaged_pixels += (uint64_t)width * height;
        if (lume->fd >= 0 && !lume->gpu)
        {
            struct lume_graphics_request request = {
                .fd = lume->fd,
                .format = LUME_GRAPHICS_XRGB8888,
                .width = width,
                .height = height,
                .pitch = lume->pitch,
                .x = box.x1,
                .y = box.y1,
                .pixels = (uintptr_t)(lume->pixels + (size_t)box.y1 * lume->width + box.x1)};
            if (graphics(LUME_GRAPHICS_PRESENT, &request) < 0)
            {
                fprintf(stderr, "wayland: screen presentation failed\n");
                lume->exit_code = 1;
                wl_display_terminate(lume->display);
                pixman_region32_fini(&damage);
                return -1;
            }
        }
    }
    if (lume->gpu && lume_weston_gpu_present(lume->gpu))
    {
        fprintf(stderr, "wayland: GPU presentation failed: %s\n", strerror(errno));
        lume->exit_code = 1;
        wl_display_terminate(lume->display);
        pixman_region32_fini(&damage);
        return -1;
    }
    lume->present_ns += monotonic_ns() - started;
    ++lume->frames;
    pixman_region32_fini(&damage);
    weston_output_arm_frame_timer(output, lume->finish_timer);
    return 0;
}

static int output_enable(struct weston_output *output)
{
    struct lume_compositor *lume = output->compositor->user_data;
    if (lume->gpu)
    {
        if (lume_weston_gpu_output_create(lume->gpu, output))
            return -1;
        lume->finish_timer =
            wl_event_loop_add_timer(wl_display_get_event_loop(lume->display), finish_frame, lume);
        if (lume->finish_timer)
            return 0;
        lume_weston_gpu_output_destroy(lume->gpu);
        return -1;
    }
    const struct pixman_renderer_interface *renderer = output->compositor->renderer->pixman;
    const struct pixman_renderer_output_options options = {
        .use_shadow = false,
        .fb_size = {lume->width, lume->height},
        .format = pixel_format_get_info(DRM_FORMAT_XRGB8888)};
    if (renderer->output_create(output, &options) < 0)
        return -1;
    lume->renderbuffer = renderer->create_image_from_ptr(output, options.format, lume->width,
                                                         lume->height, lume->pixels, lume->pitch);
    if (!lume->renderbuffer)
    {
        renderer->output_destroy(output);
        return -1;
    }
    lume->finish_timer =
        wl_event_loop_add_timer(wl_display_get_event_loop(lume->display), finish_frame, lume);
    if (lume->finish_timer)
        return 0;
    weston_renderbuffer_unref(lume->renderbuffer);
    lume->renderbuffer = NULL;
    renderer->output_destroy(output);
    return -1;
}

static int output_disable(struct weston_output *output)
{
    struct lume_compositor *lume = output->compositor->user_data;
    if (lume->gpu)
    {
        if (lume->finish_timer)
            wl_event_source_remove(lume->finish_timer);
        lume->finish_timer = NULL;
        lume->renderbuffer = NULL;
        lume_weston_gpu_output_destroy(lume->gpu);
        return 0;
    }
    if (!lume->renderbuffer)
        return 0;
    wl_event_source_remove(lume->finish_timer);
    lume->finish_timer = NULL;
    weston_renderbuffer_unref(lume->renderbuffer);
    lume->renderbuffer = NULL;
    output->compositor->renderer->pixman->output_destroy(output);
    return 0;
}

static void output_destroy(struct weston_output *output)
{
    output_disable(output);
    weston_output_release(output);
}

static void backend_destroy(struct weston_backend *backend)
{
    struct lume_compositor *lume = wl_container_of(backend, lume, backend);
    weston_head_release(&lume->head);
    wl_list_remove(&backend->link);
}

static void surface_added(struct weston_desktop_surface *desktop_surface, void *data)
{
    struct lume_compositor *lume = data;
    struct lume_window *window = calloc(1, sizeof(*window));
    struct weston_view *view = window ? weston_desktop_surface_create_view(desktop_surface) : NULL;
    if (!view)
    {
        free(window);
        weston_desktop_surface_close(desktop_surface);
        return;
    }
    window->view = view;
    weston_desktop_surface_set_user_data(desktop_surface, window);
    unsigned offset = (lume->windows++ % 8) * 24;
    weston_view_set_position(view, (struct weston_coord_global){{32 + offset, 32 + offset}});
    weston_desktop_surface_set_activated(desktop_surface, true);
}

static void surface_removed(struct weston_desktop_surface *desktop_surface, void *data)
{
    (void)data;
    struct lume_window *window = weston_desktop_surface_get_user_data(desktop_surface);
    struct weston_view *view = window ? window->view : NULL;
    if (!view)
        return;
    weston_desktop_surface_unlink_view(view);
    weston_view_destroy(view);
    free(window);
}

static void surface_committed(struct weston_desktop_surface *desktop_surface,
                              struct weston_coord_surface offset, void *data)
{
    (void)offset;
    struct lume_compositor *lume = data;
    struct weston_surface *surface = weston_desktop_surface_get_surface(desktop_surface);
    struct lume_window *window = weston_desktop_surface_get_user_data(desktop_surface);
    struct weston_view *view = window ? window->view : NULL;
    if (!view || !surface->width || !surface->height)
        return;
    lume_desktop_committed(desktop_surface);
    if (!weston_surface_is_mapped(surface))
    {
        weston_view_move_to_layer(view, &lume->layer.view_list);
        weston_desktop_surface_propagate_layer(desktop_surface);
        weston_surface_map(surface);
        weston_view_update_transform(view);
        weston_view_activate_input(view, &lume->seat, WESTON_ACTIVATE_FLAG_NONE);
    }
}

static void fullscreen_requested(struct weston_desktop_surface *surface, bool fullscreen,
                                 struct weston_output *output, void *data)
{
    (void)output;
    struct lume_compositor *lume = data;
    weston_desktop_surface_set_fullscreen(surface, fullscreen);
    weston_desktop_surface_set_size(surface, fullscreen ? lume->width : 0,
                                    fullscreen ? lume->height : 0);
    struct lume_window *window = weston_desktop_surface_get_user_data(surface);
    struct weston_view *view = window ? window->view : NULL;
    if (window)
        window->resize_edges = 0;
    if (view && fullscreen)
        weston_view_set_position(view, (struct weston_coord_global){{0, 0}});
}

static void maximized_requested(struct weston_desktop_surface *surface, bool maximized, void *data)
{
    struct lume_compositor *lume = data;
    weston_desktop_surface_set_maximized(surface, maximized);
    weston_desktop_surface_set_size(surface, maximized ? lume->width : 0,
                                    maximized ? lume->height : 0);
    struct lume_window *window = weston_desktop_surface_get_user_data(surface);
    struct weston_view *view = window ? window->view : NULL;
    if (window)
        window->resize_edges = 0;
    if (view && maximized)
        weston_view_set_position(view, (struct weston_coord_global){{0, 0}});
}

static const struct weston_desktop_api desktop_api = {.struct_size = sizeof(desktop_api),
                                                      .surface_added = surface_added,
                                                      .surface_removed = surface_removed,
                                                      .committed = surface_committed,
                                                      .move = lume_desktop_move,
                                                      .resize = lume_desktop_resize,
                                                      .fullscreen_requested = fullscreen_requested,
                                                      .maximized_requested = maximized_requested};

static const uint8_t hid_keys[256] = {[4] = KEY_A,    KEY_B,
                                      KEY_C,          KEY_D,
                                      KEY_E,          KEY_F,
                                      KEY_G,          KEY_H,
                                      KEY_I,          KEY_J,
                                      KEY_K,          KEY_L,
                                      KEY_M,          KEY_N,
                                      KEY_O,          KEY_P,
                                      KEY_Q,          KEY_R,
                                      KEY_S,          KEY_T,
                                      KEY_U,          KEY_V,
                                      KEY_W,          KEY_X,
                                      KEY_Y,          KEY_Z,
                                      [30] = KEY_1,   KEY_2,
                                      KEY_3,          KEY_4,
                                      KEY_5,          KEY_6,
                                      KEY_7,          KEY_8,
                                      KEY_9,          KEY_0,
                                      KEY_ENTER,      KEY_ESC,
                                      KEY_BACKSPACE,  KEY_TAB,
                                      KEY_SPACE,      KEY_MINUS,
                                      KEY_EQUAL,      KEY_LEFTBRACE,
                                      KEY_RIGHTBRACE, KEY_BACKSLASH,
                                      KEY_BACKSLASH,  KEY_SEMICOLON,
                                      KEY_APOSTROPHE, KEY_GRAVE,
                                      KEY_COMMA,      KEY_DOT,
                                      KEY_SLASH,      KEY_CAPSLOCK,
                                      [58] = KEY_F1,  KEY_F2,
                                      KEY_F3,         KEY_F4,
                                      KEY_F5,         KEY_F6,
                                      KEY_F7,         KEY_F8,
                                      KEY_F9,         KEY_F10,
                                      KEY_F11,        KEY_F12,
                                      KEY_SYSRQ,      KEY_SCROLLLOCK,
                                      KEY_PAUSE,      KEY_INSERT,
                                      KEY_HOME,       KEY_PAGEUP,
                                      KEY_DELETE,     KEY_END,
                                      KEY_PAGEDOWN,   KEY_RIGHT,
                                      KEY_LEFT,       KEY_DOWN,
                                      KEY_UP,         KEY_NUMLOCK,
                                      KEY_KPSLASH,    KEY_KPASTERISK,
                                      KEY_KPMINUS,    KEY_KPPLUS,
                                      KEY_KPENTER,    KEY_KP1,
                                      KEY_KP2,        KEY_KP3,
                                      KEY_KP4,        KEY_KP5,
                                      KEY_KP6,        KEY_KP7,
                                      KEY_KP8,        KEY_KP9,
                                      KEY_KP0,        KEY_KPDOT,
                                      KEY_102ND,      [224] = KEY_LEFTCTRL,
                                      KEY_LEFTSHIFT,  KEY_LEFTALT,
                                      KEY_LEFTMETA,   KEY_RIGHTCTRL,
                                      KEY_RIGHTSHIFT, KEY_RIGHTALT,
                                      KEY_RIGHTMETA};

static int input_ready(int fd, uint32_t mask, void *data)
{
    struct lume_compositor *lume = data;
    if (mask & (WL_EVENT_HANGUP | WL_EVENT_ERROR))
    {
        wl_display_terminate(lume->display);
        return 0;
    }
    struct lume_input_event events[32];
    for (unsigned batch = 0; batch < 4; ++batch)
    {
        ssize_t bytes = read(fd, events, sizeof(events));
        if (bytes < 0 && errno == EAGAIN)
            break;
        if (bytes <= 0 || bytes % sizeof(events[0]))
        {
            wl_display_terminate(lume->display);
            break;
        }
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        for (unsigned i = 0; i < (unsigned)bytes / sizeof(events[0]); ++i)
        {
            struct lume_input_event *event = events + i;
            if (event->type == LUME_INPUT_KEY && event->code < 256 && hid_keys[event->code])
            {
                bool pressed = !!event->x;
                if (lume->held[event->code] == pressed)
                    continue;
                lume->held[event->code] = pressed;
                notify_key(&lume->seat, &now, hid_keys[event->code], pressed,
                           STATE_UPDATE_AUTOMATIC);
            }
            else if (event->type == LUME_INPUT_POINTER)
            {
                struct weston_pointer_motion_event motion = {
                    .mask = WESTON_POINTER_MOTION_REL | WESTON_POINTER_MOTION_REL_UNACCEL,
                    .rel = {event->x, event->y},
                    .rel_unaccel = {event->x, event->y}};
                notify_motion(&lume->seat, &now, &motion);
                unsigned changed = lume->buttons ^ event->buttons;
                const unsigned codes[] = {BTN_LEFT, BTN_RIGHT, BTN_MIDDLE};
                for (unsigned b = 0; b < 3; ++b)
                    if (changed & (1u << b))
                        notify_button(&lume->seat, &now, codes[b], !!(event->buttons & (1u << b)));
                lume->buttons = event->buttons & 7u;
                notify_pointer_frame(&lume->seat);
            }
            else if (event->type == LUME_INPUT_RESET)
            {
                for (unsigned key = 0; key < 256; ++key)
                    if (lume->held[key])
                    {
                        notify_key(&lume->seat, &now, hid_keys[key], WL_KEYBOARD_KEY_STATE_RELEASED,
                                   STATE_UPDATE_AUTOMATIC);
                        lume->held[key] = false;
                    }
                const unsigned codes[] = {BTN_LEFT, BTN_RIGHT, BTN_MIDDLE};
                for (unsigned b = 0; b < 3; ++b)
                    if (lume->buttons & (1u << b))
                        notify_button(&lume->seat, &now, codes[b],
                                      WL_POINTER_BUTTON_STATE_RELEASED);
                lume->buttons = 0;
            }
        }
    }
    return 0;
}

static void compositor_exit(struct weston_compositor *compositor)
{
    wl_display_terminate(compositor->wl_display);
}

static int terminate_signal(int signal, void *data)
{
    (void)signal;
    wl_display_terminate(data);
    return 0;
}

static int terminate_timer(void *data)
{
    wl_display_terminate(data);
    return 0;
}

static int child_signal(int signal, void *data)
{
    (void)signal;
    (void)data;
    while (waitpid(-1, NULL, WNOHANG) > 0)
    {
    }
    return 0;
}

int main(int argc, char **argv)
{
    struct lume_compositor lume = {.fd = -1, .width = 640, .height = 480};
    bool headless = false;
    int duration = 0;
    const char *socket = "wayland-0";
    const char *renderer = "auto";
    char **command = NULL;
    for (int i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "--") && i + 1 < argc)
        {
            command = argv + i + 1;
            break;
        }
        else if (!strcmp(argv[i], "--headless"))
            headless = true;
        else if (!strcmp(argv[i], "--renderer") && i + 1 < argc)
        {
            renderer = argv[++i];
            if (strcmp(renderer, "auto") && strcmp(renderer, "gl") && strcmp(renderer, "pixman"))
                return 2;
        }
        else if (!strcmp(argv[i], "--socket") && i + 1 < argc)
            socket = argv[++i];
        else if (!strcmp(argv[i], "--run-for-ms") && i + 1 < argc)
        {
            char *end;
            long value = strtol(argv[++i], &end, 10);
            if (*end || value < 1 || value > 3600000)
                return 2;
            duration = value;
        }
        else
        {
            fprintf(
                stderr,
                "Usage: wayland [--headless] [--renderer auto|gl|pixman] [--socket NAME] [--run-for-ms N] [-- COMMAND...]\n");
            return 2;
        }
    }
    if (lume_wayland_runtime() || setenv("WAYLAND_DISPLAY", socket, 1))
    {
        fprintf(stderr, "wayland: invalid runtime directory: %s\n", strerror(errno));
        return 1;
    }
    weston_log_set_handler(log_message, log_message);
    if (!headless)
    {
        struct lume_graphics_request request = {0};
        if (graphics(LUME_GRAPHICS_ACQUIRE, &request) < 0)
        {
            fprintf(stderr, "wayland: cannot acquire screen and input\n");
            return 1;
        }
        lume.fd = request.fd;
        lume.width = request.width;
        lume.height = request.height;
        fcntl(lume.fd, F_SETFL, fcntl(lume.fd, F_GETFL) | O_NONBLOCK);
        fcntl(lume.fd, F_SETFD, FD_CLOEXEC);
    }
    if (!lume.width || !lume.height || lume.width > 16384 || lume.height > 16384)
        goto fail;
    lume.display = wl_display_create();
    struct weston_log_context *log = weston_log_ctx_create();
    if (!lume.display || !log)
        goto fail;
    lume.compositor = weston_compositor_create(lume.display, log, &lume, NULL);
    if (!lume.compositor)
        goto fail;
    lume.compositor->exit = compositor_exit;
    lume.compositor->idle_time = 0;
    lume.backend.destroy = backend_destroy;
    lume.backend.supported_presentation_clocks = WESTON_PRESENTATION_CLOCKS_SOFTWARE;
    wl_list_insert(&lume.compositor->backend_list, &lume.backend.link);
    if (strcmp(renderer, "pixman"))
        lume.gpu = lume_weston_gpu_create(lume.compositor, lume.fd, &lume.width, &lume.height);
    if (!lume.gpu && (!strcmp(renderer, "gl") || (strcmp(renderer, "pixman") && errno == EIO)))
        goto fail;
    if (!lume.gpu)
    {
        if (pixman_renderer_init(lume.compositor) < 0)
            goto fail;
        lume.pitch = lume.width * 4;
        if (posix_memalign((void **)&lume.pixels, 4096, (size_t)lume.pitch * lume.height))
            goto fail;
        memset(lume.pixels, 0, (size_t)lume.pitch * lume.height);
    }
    if (weston_compositor_backends_loaded(lume.compositor) < 0)
        goto fail;
    weston_head_init(&lume.head, "Lume");
    lume.head.backend = &lume.backend;
    weston_head_set_monitor_strings(&lume.head, "Lume", "Native display", NULL);
    weston_head_set_connection_status(&lume.head, true);
    weston_head_set_physical_size(&lume.head, 0, 0);
    weston_compositor_add_head(lume.compositor, &lume.head);
    weston_output_init(&lume.output, lume.compositor, "Lume");
    lume.output.backend = &lume.backend;
    lume.output.enable = output_enable;
    lume.output.disable = output_disable;
    lume.output.destroy = output_destroy;
    lume.output.start_repaint_loop = start_repaint;
    lume.output.repaint = repaint;
    weston_compositor_add_pending_output(&lume.output, lume.compositor);
    if (weston_output_attach_head(&lume.output, &lume.head) < 0)
        goto fail;
    weston_output_set_scale(&lume.output, 1);
    weston_output_set_transform(&lume.output, WL_OUTPUT_TRANSFORM_NORMAL);
    lume.mode.width = lume.width;
    lume.mode.height = lume.height;
    lume.mode.refresh = 60000;
    lume.mode.flags = WL_OUTPUT_MODE_CURRENT | WL_OUTPUT_MODE_PREFERRED;
    wl_list_insert(&lume.output.mode_list, &lume.mode.link);
    lume.output.current_mode = &lume.mode;
    if (weston_output_enable(&lume.output) < 0)
        goto fail;
    struct xkb_rule_names names = {.rules = "evdev", .model = "pc105", .layout = "pt"};
    if (weston_compositor_set_xkb_rule_names(lume.compositor, &names) < 0)
        goto fail;
    weston_seat_init(&lume.seat, lume.compositor, "seat0");
    if (weston_seat_init_pointer(&lume.seat) < 0 || weston_seat_init_keyboard(&lume.seat, NULL) < 0)
        goto fail;
    weston_layer_init(&lume.layer, lume.compositor);
    weston_layer_set_position(&lume.layer, WESTON_LAYER_POSITION_NORMAL);
    weston_compositor_add_button_binding(lume.compositor, BTN_LEFT, 0, lume_desktop_focus,
                                         &lume.layer);
    weston_layer_init(&lume.background_layer, lume.compositor);
    weston_layer_set_position(&lume.background_layer, WESTON_LAYER_POSITION_BACKGROUND);
    lume.background = weston_surface_create(lume.compositor);
    lume.background_view = weston_view_create(lume.background);
    lume.background_buffer =
        weston_buffer_create_solid_rgba(lume.compositor, 0.07, 0.10, 0.15, 1.0);
    if (!lume.background || !lume.background_view || !lume.background_buffer)
        goto fail;
    weston_surface_attach_solid(lume.background, lume.background_buffer, lume.width, lume.height);
    pixman_region32_fini(&lume.background->opaque);
    pixman_region32_init_rect(&lume.background->opaque, 0, 0, lume.width, lume.height);
    weston_view_move_to_layer(lume.background_view, &lume.background_layer.view_list);
    weston_view_set_position(lume.background_view, (struct weston_coord_global){{0, 0}});
    weston_surface_map(lume.background);
    lume.desktop = weston_desktop_create(lume.compositor, &desktop_api, &lume);
    struct wl_event_loop *loop = wl_display_get_event_loop(lume.display);
    if (!lume.desktop || wl_display_add_socket(lume.display, socket) < 0)
        goto fail;
    if (lume.fd >= 0)
    {
        lume.input_source =
            wl_event_loop_add_fd(loop, lume.fd, WL_EVENT_READABLE, input_ready, &lume);
        if (!lume.input_source)
            goto fail;
    }
    struct wl_event_source *interrupt =
        wl_event_loop_add_signal(loop, SIGINT, terminate_signal, lume.display);
    struct wl_event_source *terminate =
        wl_event_loop_add_signal(loop, SIGTERM, terminate_signal, lume.display);
    struct wl_event_source *children = wl_event_loop_add_signal(loop, SIGCHLD, child_signal, NULL);
    struct wl_event_source *timer = NULL;
    if (duration)
    {
        timer = wl_event_loop_add_timer(loop, terminate_timer, lume.display);
        if (!timer || wl_event_source_timer_update(timer, duration) < 0)
            goto fail;
    }
    if (!interrupt || !terminate || !children)
        goto fail;
    weston_compositor_wake(lume.compositor);
    printf("WAYLAND_READY socket=%s renderer=%s width=%u height=%u\n", socket,
           lume.gpu ? "gl" : "pixman", lume.width, lume.height);
    if (lume.gpu)
        printf("WAYLAND_GPU device=%s buffers=2 readback=0\n", lume_weston_gpu_name(lume.gpu));
    fflush(stdout);
    if (command)
    {
        pid_t child = fork();
        if (child < 0)
            goto fail;
        if (!child)
        {
            execvp(command[0], command);
            fprintf(stderr, "wayland: cannot execute %s: %s\n", command[0], strerror(errno));
            _exit(127);
        }
    }
    wl_display_run(lume.display);
    printf("WAYLAND_STOP frames=%llu damaged_pixels=%llu render_ns=%llu present_ns=%llu\n",
           (unsigned long long)lume.frames, (unsigned long long)lume.damaged_pixels,
           (unsigned long long)lume.render_ns, (unsigned long long)lume.present_ns);
    wl_display_destroy_clients(lume.display);
    if (timer)
        wl_event_source_remove(timer);
    wl_event_source_remove(interrupt);
    wl_event_source_remove(terminate);
    wl_event_source_remove(children);
    if (lume.input_source)
        wl_event_source_remove(lume.input_source);
    weston_desktop_destroy(lume.desktop);
    weston_surface_unref(lume.background);
    weston_buffer_destroy_solid(lume.background_buffer);
    weston_layer_fini(&lume.layer);
    weston_layer_fini(&lume.background_layer);
    weston_seat_release(&lume.seat);
    weston_compositor_destroy(lume.compositor);
    lume_weston_gpu_destroy(lume.gpu);
    wl_display_destroy(lume.display);
    weston_log_ctx_destroy(log);
    free(lume.pixels);
    if (lume.fd >= 0)
        close(lume.fd);
    return lume.exit_code;
fail:
    fprintf(stderr, "wayland: initialization failed: %s\n", strerror(errno));
    if (lume.fd >= 0)
        close(lume.fd);
    return 1;
}
