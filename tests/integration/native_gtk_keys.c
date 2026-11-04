/* SPDX-License-Identifier: BSD-3-Clause */
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/XKBlib.h>
#include <stdio.h>
#include <string.h>
#include "libxfce4kbd-private/xfce-shortcuts-grabber.h"
#include "libxfce4ui/libxfce4ui.h"

static unsigned received;

static void activated(XfceShortcutsGrabber *grabber, const char *shortcut, gint timestamp,
                      gpointer context)
{
    (void)grabber;
    (void)timestamp;
    (void)context;
    printf("GTK_KEY_ACTIVATED %s\n", shortcut);
    fflush(stdout);
    if (!strcmp(shortcut, "<Super>e"))
        received |= 1;
    if (!strcmp(shortcut, "<Primary><Alt>t"))
        received |= 2;
    if (received == 3)
        gtk_main_quit();
}

static gboolean expired(gpointer context)
{
    (void)context;
    gtk_main_quit();
    return G_SOURCE_REMOVE;
}

int main(int argc, char **argv)
{
    if (!gtk_init_check(&argc, &argv))
        return 1;
    GError *error = NULL;
    GBytes *resource = g_resources_lookup_data("/org/xfce/libxfce4ui/libxfce4ui-dialog-ui.ui",
                                               G_RESOURCE_LOOKUP_FLAGS_NONE, &error);
    if (!resource || error)
        return 1;
    g_bytes_unref(resource);
    GtkWidget *dialog =
        xfce_message_dialog_new(NULL, "Dialog check", "dialog-information", "Dialog resource", NULL,
                                "Close", GTK_RESPONSE_CLOSE, NULL);
    if (!GTK_IS_MESSAGE_DIALOG(dialog))
        return 1;
    gtk_widget_destroy(dialog);
    puts("GTK_UI_RESOURCE_PASS");
    Display *display = gdk_x11_get_default_xdisplay();
    GdkKeymap *map = gdk_keymap_get_for_display(gdk_display_get_default());
    gdk_keymap_have_bidi_layouts(map);
    GdkModifierType virtual = GDK_SUPER_MASK;
    gboolean mapped = gdk_keymap_map_virtual_modifiers(map, &virtual);
    printf("GTK_SUPER mapped=%d modifiers=%x\n", mapped, (unsigned)virtual);
    XModifierKeymap *modifiers = XGetModifierMapping(display);
    if (!modifiers)
        return 1;
    for (unsigned i = 0; i < 8 * (unsigned)modifiers->max_keypermod; ++i)
        if (modifiers->modifiermap[i])
            printf("GTK_MODIFIER index=%u code=%u symbol=%lx\n", i / modifiers->max_keypermod,
                   modifiers->modifiermap[i],
                   XkbKeycodeToKeysym(display, modifiers->modifiermap[i], 0, 0));
    XFreeModifiermap(modifiers);
    XkbDescPtr keyboard =
        XkbGetMap(display, XkbAllClientInfoMask | XkbVirtualModsMask, XkbUseCoreKbd);
    if (!keyboard || XkbGetNames(display, XkbVirtualModNamesMask, keyboard) != Success)
        return 1;
    for (unsigned i = 0; i < XkbNumVirtualMods; ++i)
        if (keyboard->names->vmods[i])
        {
            char *name = XGetAtomName(display, keyboard->names->vmods[i]);
            printf("GTK_VIRTUAL name=%s real=%x\n", name ? name : "", keyboard->server->vmods[i]);
            XFree(name);
        }
    XkbFreeKeyboard(keyboard, XkbAllComponentsMask, True);
    XfceShortcutsGrabber *grabber = xfce_shortcuts_grabber_new();
    g_signal_connect(grabber, "shortcut-activated", G_CALLBACK(activated), NULL);
    xfce_shortcuts_grabber_add(grabber, "<Super>e");
    xfce_shortcuts_grabber_add(grabber, "<Primary><Alt>t");
    g_timeout_add_seconds(30, expired, NULL);
    puts("GTK_KEYS_READY");
    fflush(stdout);
    gtk_main();
    g_object_unref(grabber);
    if (received != 3 || !mapped || !(virtual & GDK_MOD4_MASK))
        return 1;
    puts("GTK_KEYS_PASS");
    return 0;
}
