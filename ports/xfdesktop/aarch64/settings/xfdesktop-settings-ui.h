#ifndef __RESOURCE_xfdesktop_settings_ui_H__
#define __RESOURCE_xfdesktop_settings_ui_H__

#include <gio/gio.h>

G_GNUC_INTERNAL GResource *xfdesktop_settings_ui_get_resource (void);

G_GNUC_INTERNAL void xfdesktop_settings_ui_register_resource (void);
G_GNUC_INTERNAL void xfdesktop_settings_ui_unregister_resource (void);

#endif
