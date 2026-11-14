/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_WESTON_SHELL_H
#define LUME_WESTON_SHELL_H
#include <libweston/desktop.h>

struct lume_window
{
    struct weston_view *view;
    struct weston_coord_global resize_origin;
    struct weston_size resize_size;
    unsigned resize_edges;
};

void lume_desktop_committed(struct weston_desktop_surface *);
void lume_desktop_move(struct weston_desktop_surface *, struct weston_seat *, uint32_t, void *);
void lume_desktop_resize(struct weston_desktop_surface *, struct weston_seat *, uint32_t,
                         enum weston_desktop_surface_edge, void *);
void lume_desktop_focus(struct weston_pointer *, const struct timespec *, uint32_t, void *);
#endif
