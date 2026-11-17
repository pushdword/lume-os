/* SPDX-License-Identifier: BSD-3-Clause */
#include <glib.h>
#include <dbus/dbus.h>
#include <stddef.h>
#include <stdint.h>

_Static_assert(sizeof(gint64) == 8, "GLib signed integer width");
_Static_assert(sizeof(guint64) == 8, "GLib unsigned integer width");
_Static_assert(sizeof(goffset) == 8, "GLib file offset width");
_Static_assert(sizeof(gsize) == sizeof(size_t), "GLib size width");
_Static_assert(sizeof(gssize) == sizeof(ptrdiff_t), "GLib signed size width");
_Static_assert(sizeof(gintptr) == sizeof(void *), "GLib pointer integer width");
_Static_assert(GLIB_SIZEOF_VOID_P == sizeof(void *), "GLib pointer width");
_Static_assert(GLIB_SIZEOF_LONG == sizeof(long), "GLib long width");
_Static_assert(sizeof(dbus_int64_t) == 8, "D-Bus signed integer width");
_Static_assert(sizeof(dbus_uint64_t) == 8, "D-Bus unsigned integer width");
_Static_assert(DBUS_SIZEOF_VOID_P == sizeof(void *), "D-Bus pointer width");
_Static_assert(GUINT64_SWAP_LE_BE_CONSTANT(G_GUINT64_CONSTANT(0x1122334455667788)) ==
                   G_GUINT64_CONSTANT(0x8877665544332211),
               "GLib 64-bit byte order");
