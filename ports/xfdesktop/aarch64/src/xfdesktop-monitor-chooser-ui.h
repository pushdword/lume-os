#ifndef __RESOURCE_xfdesktop_monitor_chooser_ui_H__
#define __RESOURCE_xfdesktop_monitor_chooser_ui_H__

#include <gio/gio.h>

G_GNUC_INTERNAL GResource *xfdesktop_monitor_chooser_ui_get_resource (void);

G_GNUC_INTERNAL void xfdesktop_monitor_chooser_ui_register_resource (void);
G_GNUC_INTERNAL void xfdesktop_monitor_chooser_ui_unregister_resource (void);

#endif
