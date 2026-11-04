/* SPDX-License-Identifier: BSD-3-Clause */
#include <gtk/gtk.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned draws;
static unsigned activations;
static GtkWidget *entry;

static gboolean ready(gpointer data)
{
    (void)data;
    gdk_display_sync(gdk_display_get_default());
    puts("GTK_READY");
    fflush(stdout);
    return G_SOURCE_REMOVE;
}

static gboolean drawn(GtkWidget *widget, cairo_t *context, gpointer data)
{
    (void)widget;
    (void)context;
    (void)data;
    ++draws;
    if (draws == 1)
    {
        g_idle_add(ready, NULL);
    }
    return FALSE;
}

static void activated(GtkButton *button, gpointer data)
{
    (void)button;
    (void)data;
    assert(draws);
    if (strcmp(gtk_entry_get_text(GTK_ENTRY(entry)), "ação, café e informação"))
        fprintf(stderr, "GTK_INPUT_MISMATCH value=<%s>\n", gtk_entry_get_text(GTK_ENTRY(entry)));
    assert(!strcmp(gtk_entry_get_text(GTK_ENTRY(entry)), "ação, café e informação"));
    ++activations;
    puts("GTK_INPUT_PASS");
    fflush(stdout);
    gtk_main_quit();
}

static gboolean expired(gpointer data)
{
    (void)data;
    fprintf(stderr, "GTK input timeout after %u draws\n", draws);
    gtk_main_quit();
    return G_SOURCE_REMOVE;
}

int main(int argc, char **argv)
{
    puts("GTK_START");
    fflush(stdout);
    assert(gtk_init_check(&argc, &argv));
    puts("GTK_INIT_PASS");
    fflush(stdout);
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "GTK native check");
    gtk_window_set_default_size(GTK_WINDOW(window), 640, 320);
    gtk_container_set_border_width(GTK_CONTAINER(window), 24);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_container_add(GTK_CONTAINER(window), box);
    GtkWidget *label = gtk_label_new("Ação, café e informação em português");
    gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
    entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), "Substituir este texto");
    gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
    gtk_box_pack_start(GTK_BOX(box), entry, FALSE, TRUE, 0);
    GtkWidget *button = gtk_button_new_with_label("Validar entrada");
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, TRUE, 0);
    g_signal_connect(window, "draw", G_CALLBACK(drawn), NULL);
    g_signal_connect(button, "clicked", G_CALLBACK(activated), NULL);
    gtk_window_set_default(GTK_WINDOW(window), button);
    gtk_widget_set_can_default(button, TRUE);
    gtk_widget_grab_default(button);
    gtk_widget_show_all(window);
    gtk_widget_grab_focus(entry);
    g_timeout_add_seconds(60, expired, NULL);
    gtk_main();
    gtk_widget_destroy(window);
    assert(draws && activations == 1);
    puts("GTK_NATIVE_PASS");
    return 0;
}
