/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_WESTON_LAYER_H
#define LUME_WESTON_LAYER_H
#include <libweston/libweston.h>
#include "layer-layout.h"

struct lume_layer_shell;
struct lume_layer_shell *lume_layer_shell_create(struct weston_compositor *, struct weston_output *,
                                                 struct weston_seat *, void (*)(void *), void *);
void lume_layer_shell_destroy(struct lume_layer_shell *);
struct lume_layer_rect lume_layer_shell_work_area(struct lume_layer_shell *);
#endif
