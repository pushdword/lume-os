/* SPDX-License-Identifier: BSD-3-Clause */
#include "panel-builtins.h"

#define PLUGIN(name) extern GType lume_panel_##name##_init(GTypeModule *, gboolean *)
PLUGIN(actions);
PLUGIN(applicationsmenu);
PLUGIN(clock);
PLUGIN(directorymenu);
PLUGIN(launcher);
PLUGIN(pager);
PLUGIN(separator);
PLUGIN(showdesktop);
PLUGIN(systray);
PLUGIN(tasklist);
PLUGIN(windowmenu);
#undef PLUGIN

PluginInitFunc lume_panel_builtin(const gchar *name)
{
    static const struct
    {
        const gchar *name;
        PluginInitFunc initialize;
    } plugins[] = {

#define PLUGIN(name) {#name, lume_panel_##name##_init}
        PLUGIN(actions),       PLUGIN(applicationsmenu), PLUGIN(clock),
        PLUGIN(directorymenu), PLUGIN(launcher),         PLUGIN(pager),
        PLUGIN(separator),     PLUGIN(showdesktop),      PLUGIN(systray),
        PLUGIN(tasklist),      PLUGIN(windowmenu)
#undef PLUGIN
    };
    for (gsize i = 0; i < G_N_ELEMENTS(plugins); ++i)
        if (g_strcmp0(name, plugins[i].name) == 0)
            return plugins[i].initialize;
    return NULL;
}
