/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_WESTON_POPUP_H
#define LUME_WESTON_POPUP_H
#include <libweston/libweston.h>
bool lume_desktop_popup_set_layer_parent(struct wl_resource *, struct weston_view *);
#endif
