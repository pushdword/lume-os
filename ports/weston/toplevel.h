/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_WESTON_TOPLEVEL_H
#define LUME_WESTON_TOPLEVEL_H
#include <libweston/libweston.h>
#include <libweston/desktop.h>

enum lume_toplevel_action
{
    LUME_TOPLEVEL_ACTIVATE,
    LUME_TOPLEVEL_MINIMIZE,
    LUME_TOPLEVEL_MAXIMIZE,
    LUME_TOPLEVEL_FULLSCREEN,
    LUME_TOPLEVEL_CLOSE
};
struct lume_toplevel_manager;
struct lume_toplevel;
struct lume_toplevel_manager *lume_toplevel_manager_create(
    struct weston_compositor *, struct weston_head *, struct weston_seat *,
    void (*)(struct weston_desktop_surface *, enum lume_toplevel_action, bool, void *), void *);
void lume_toplevel_manager_destroy(struct lume_toplevel_manager *);
struct lume_toplevel *lume_toplevel_add(struct lume_toplevel_manager *,
                                        struct weston_desktop_surface *);
void lume_toplevel_remove(struct lume_toplevel *);
void lume_toplevel_update(struct lume_toplevel *, bool);
#endif
