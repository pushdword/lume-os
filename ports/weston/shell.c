/* SPDX-License-Identifier: BSD-3-Clause */
#include "config.h"
#include "shell.h"
#include <stdlib.h>

struct window_grab
{
    struct weston_pointer_grab base;
    struct wl_listener destroyed;
    struct weston_view *view;
    struct weston_desktop_surface *surface;
    struct weston_coord_global origin, pointer;
    struct weston_size size;
    unsigned edges;
};

static void grab_cancel(struct weston_pointer_grab *base)
{
    struct window_grab *grab = wl_container_of(base, grab, base);
    weston_pointer_end_grab(base->pointer);
    wl_list_remove(&grab->destroyed.link);
    if (grab->edges)
        weston_desktop_surface_set_resizing(grab->surface, false);
    free(grab);
}

static void view_destroyed(struct wl_listener *listener, void *data)
{
    (void)data;
    struct window_grab *grab = wl_container_of(listener, grab, destroyed);
    grab_cancel(&grab->base);
}

static void grab_focus(struct weston_pointer_grab *base)
{
    (void)base;
}

static void grab_motion(struct weston_pointer_grab *base, const struct timespec *time,
                        struct weston_pointer_motion_event *event)
{
    (void)time;
    struct window_grab *grab = wl_container_of(base, grab, base);
    weston_pointer_move(base->pointer, event);
    double dx = base->pointer->pos.c.x - grab->pointer.c.x;
    double dy = base->pointer->pos.c.y - grab->pointer.c.y;
    if (!grab->edges)
    {
        struct weston_coord_global pos = grab->origin;
        pos.c.x += dx;
        pos.c.y += dy;
        weston_view_set_position(grab->view, pos);
        weston_view_update_transform(grab->view);
        weston_surface_damage(grab->view->surface);
        return;
    }
    int width = grab->size.width, height = grab->size.height;
    if (grab->edges & WESTON_DESKTOP_SURFACE_EDGE_LEFT)
        width -= dx;
    if (grab->edges & WESTON_DESKTOP_SURFACE_EDGE_RIGHT)
        width += dx;
    if (grab->edges & WESTON_DESKTOP_SURFACE_EDGE_TOP)
        height -= dy;
    if (grab->edges & WESTON_DESKTOP_SURFACE_EDGE_BOTTOM)
        height += dy;
    struct weston_size minimum = weston_desktop_surface_get_min_size(grab->surface);
    struct weston_size maximum = weston_desktop_surface_get_max_size(grab->surface);
    if (width < 1)
        width = 1;
    if (height < 1)
        height = 1;
    if (width < minimum.width)
        width = minimum.width;
    if (height < minimum.height)
        height = minimum.height;
    if (maximum.width && width > maximum.width)
        width = maximum.width;
    if (maximum.height && height > maximum.height)
        height = maximum.height;
    weston_desktop_surface_set_size(grab->surface, width, height);
}

static void grab_button(struct weston_pointer_grab *base, const struct timespec *time,
                        uint32_t button, uint32_t state)
{
    (void)time;
    (void)button;
    if (!base->pointer->button_count && state == WL_POINTER_BUTTON_STATE_RELEASED)
        grab_cancel(base);
}

static void grab_axis(struct weston_pointer_grab *base, const struct timespec *time,
                      struct weston_pointer_axis_event *event)
{
    (void)base;
    (void)time;
    (void)event;
}

static void grab_axis_source(struct weston_pointer_grab *base, uint32_t source)
{
    (void)base;
    (void)source;
}

static const struct weston_pointer_grab_interface grab_interface = {.focus = grab_focus,
                                                                    .motion = grab_motion,
                                                                    .button = grab_button,
                                                                    .axis = grab_axis,
                                                                    .axis_source = grab_axis_source,
                                                                    .frame = grab_focus,
                                                                    .cancel = grab_cancel};

static void start_grab(struct weston_desktop_surface *surface, struct weston_seat *seat,
                       uint32_t serial, unsigned edges)
{
    struct weston_pointer *pointer = weston_seat_get_pointer(seat);
    struct lume_window *window = weston_desktop_surface_get_user_data(surface);
    struct weston_view *view = window ? window->view : NULL;
    if (!pointer || !view || !pointer->focus || pointer->focus->surface != view->surface ||
        !pointer->button_count || pointer->grab_serial != serial ||
        pointer->grab != &pointer->default_grab || weston_desktop_surface_get_fullscreen(surface) ||
        weston_desktop_surface_get_maximized(surface))
        return;
    struct window_grab *grab = calloc(1, sizeof(*grab));
    if (!grab)
        return;
    grab->base.interface = &grab_interface;
    grab->view = view;
    grab->surface = surface;
    grab->origin = weston_view_get_pos_offset_global(view);
    grab->pointer = pointer->pos;
    struct weston_geometry geometry = weston_desktop_surface_get_geometry(surface);
    grab->size = (struct weston_size){geometry.width, geometry.height};
    grab->edges = edges;
    window->resize_edges = edges;
    window->resize_origin = grab->origin;
    window->resize_size = grab->size;
    grab->destroyed.notify = view_destroyed;
    wl_signal_add(&view->unmap_signal, &grab->destroyed);
    if (edges)
        weston_desktop_surface_set_resizing(surface, true);
    weston_pointer_start_grab(pointer, &grab->base);
}

void lume_desktop_move(struct weston_desktop_surface *surface, struct weston_seat *seat,
                       uint32_t serial, void *data)
{
    (void)data;
    start_grab(surface, seat, serial, 0);
}

void lume_desktop_resize(struct weston_desktop_surface *surface, struct weston_seat *seat,
                         uint32_t serial, enum weston_desktop_surface_edge edges, void *data)
{
    (void)data;
    start_grab(surface, seat, serial, edges);
}

void lume_desktop_focus(struct weston_pointer *pointer, const struct timespec *time,
                        uint32_t button, void *data)
{
    (void)time;
    (void)button;
    struct weston_layer *layer = data;
    struct weston_view *view = pointer->focus;
    if (!view || !weston_surface_is_desktop_surface(view->surface))
        return;
    weston_view_move_to_layer(view, &layer->view_list);
    weston_view_activate_input(view, pointer->seat, WESTON_ACTIVATE_FLAG_CLICKED);
    weston_desktop_surface_set_activated(weston_surface_get_desktop_surface(view->surface), true);
}

void lume_desktop_committed(struct weston_desktop_surface *surface)
{
    struct lume_window *window = weston_desktop_surface_get_user_data(surface);
    if (!window || !window->resize_edges)
        return;
    struct weston_geometry geometry = weston_desktop_surface_get_geometry(surface);
    struct weston_coord_global pos = window->resize_origin;
    if (window->resize_edges & WESTON_DESKTOP_SURFACE_EDGE_LEFT)
        pos.c.x += window->resize_size.width - geometry.width;
    if (window->resize_edges & WESTON_DESKTOP_SURFACE_EDGE_TOP)
        pos.c.y += window->resize_size.height - geometry.height;
    weston_view_set_position(window->view, pos);
    weston_view_update_transform(window->view);
    if (!weston_desktop_surface_get_resizing(surface))
        window->resize_edges = 0;
}
