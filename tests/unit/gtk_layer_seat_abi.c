/* SPDX-License-Identifier: BSD-3-Clause */
#include "../../vendor/gtk/gdk/wayland/gdkdevice-wayland.c"
#include <gtk/gtk.h>
#include "../../vendor/gtk-layer-shell/gtk-priv/h/gdk_wayland_pointer_frame_data_priv.h"
#include "../../vendor/gtk-layer-shell/gtk-priv/h/gdk_wayland_pointer_data_priv.h"
#include "../../vendor/gtk-layer-shell/gtk-priv/h/gdk_wayland_touch_data_priv.h"
#include "../../vendor/gtk-layer-shell/gtk-priv/h/gdk_wayland_tablet_data_priv.h"
#include "../../vendor/gtk-layer-shell/gtk-priv/h/gdk_wayland_seat_priv.h"
#include <stddef.h>

#define CHECK_FIELD(type, reference, field)                                                        \
    _Static_assert(offsetof(type, field) == offsetof(reference, field), #type "." #field)

CHECK_FIELD(GdkWaylandSeat, struct _GdkWaylandSeat_v3_24_49, keyboard_key_serial);
CHECK_FIELD(GdkWaylandSeat, struct _GdkWaylandSeat_v3_24_49, pointer_info);
CHECK_FIELD(GdkWaylandSeat, struct _GdkWaylandSeat_v3_24_49, touches);
CHECK_FIELD(GdkWaylandSeat, struct _GdkWaylandSeat_v3_24_49, tablets);
CHECK_FIELD(GdkWaylandPointerData, struct _GdkWaylandPointerData_v3_24_49, press_serial);
CHECK_FIELD(GdkWaylandTouchData, struct _GdkWaylandTouchData_v3_22_0, touch_down_serial);
CHECK_FIELD(GdkWaylandTabletData, struct _GdkWaylandTabletData_v3_24_51, pointer_info);
