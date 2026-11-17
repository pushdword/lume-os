/* SPDX-License-Identifier: BSD-3-Clause */
#include "../../vendor/gtk/gdk/wayland/gdkwindow-wayland.c"
#include <gtk/gtk.h>
#include "../../vendor/gtk-layer-shell/gtk-priv/h/gdk_window_impl_priv.h"
#include "../../vendor/gtk-layer-shell/gtk-priv/h/gdk_window_priv.h"
#include "../../vendor/gtk-layer-shell/gtk-priv/h/gdk_window_impl_class_priv.h"
#include "../../vendor/gtk-layer-shell/gtk-priv/h/gdk_window_impl_wayland_priv.h"
#include <stddef.h>

#define CHECK_FIELD(type, reference, field)                                                        \
    _Static_assert(offsetof(type, field) == offsetof(reference, field), #type "." #field)

CHECK_FIELD(GdkWindow, struct _GdkWindow_v3_24_11, impl);
CHECK_FIELD(GdkWindow, struct _GdkWindow_v3_24_11, transient_for);
CHECK_FIELD(GdkWindowImplClass, struct _GdkWindowImplClass_v3_24_35, move_to_rect);
CHECK_FIELD(GdkWindowImplWayland, struct _GdkWindowImplWayland_v3_24_44, transient_for);
CHECK_FIELD(GdkWindowImplWayland, struct _GdkWindowImplWayland_v3_24_44, grab_input_seat);
CHECK_FIELD(GdkWindowImplWayland, struct _GdkWindowImplWayland_v3_24_44, margin_top);
CHECK_FIELD(GdkWindowImplWayland, struct _GdkWindowImplWayland_v3_24_44, margin_left);
CHECK_FIELD(GdkWindowImplWayland, struct _GdkWindowImplWayland_v3_24_44, margin_bottom);
CHECK_FIELD(GdkWindowImplWayland, struct _GdkWindowImplWayland_v3_24_44, margin_right);

#define CHECK_BITS(field)                                                                          \
    const GdkWindowImplWayland actual_##field                                                      \
        __attribute__((used, section(".lume.abi.actual." #field))) = {.field = 1};                 \
    const struct _GdkWindowImplWayland_v3_24_44 reference_##field                                  \
        __attribute__((used, section(".lume.abi.reference." #field))) = {.field = 1}

CHECK_BITS(mapped);
CHECK_BITS(pending_buffer_attached);
CHECK_BITS(pending_commit);
