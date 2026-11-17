/* SPDX-License-Identifier: BSD-3-Clause */
#include <gtk/gtk.h>
#include <gdk/gdkwayland.h>
#include <epoxy/gl.h>
#include <stdio.h>
#include <string.h>

static int result = 1;
static unsigned rendered;

static gboolean finish(gpointer data)
{
    (void)data;
    gtk_main_quit();
    return G_SOURCE_REMOVE;
}

static gboolean render(GtkGLArea *area, GdkGLContext *context, gpointer data)
{
    (void)context;
    (void)data;
    if (rendered++)
        return TRUE;
    if (gtk_gl_area_get_error(area))
        return FALSE;
    const char *renderer = (const char *)glGetString(GL_RENDERER);
    const char *version = (const char *)glGetString(GL_VERSION);
    unsigned char pixel[4] = {0};
    glClearColor(32.0f / 255.0f, 128.0f / 255.0f, 191.0f / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glReadPixels(16, 16, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    GLenum error = glGetError();
    result = !renderer || !strstr(renderer, "V3D") || error || pixel[0] != 32 || pixel[1] != 128 ||
             pixel[2] != 191 || pixel[3] != 255;
    printf("GTK_WAYLAND_GPU_%s renderer=%s version=%s pixel=%u,%u,%u,%u error=%u\n",
           result ? "FAIL" : "PASS", renderer ? renderer : "none", version ? version : "none",
           pixel[0], pixel[1], pixel[2], pixel[3], error);
    fflush(stdout);
    g_timeout_add(400, finish, NULL);
    return TRUE;
}

int main(int argc, char **argv)
{
    if (!gtk_init_check(&argc, &argv) || !GDK_IS_WAYLAND_DISPLAY(gdk_display_get_default()))
    {
        fprintf(stderr, "GTK_WAYLAND_GPU_FAIL display\n");
        return 1;
    }
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "GTK Wayland GPU");
    gtk_window_set_default_size(GTK_WINDOW(window), 320, 240);
    GtkWidget *area = gtk_gl_area_new();
    gtk_gl_area_set_use_es(GTK_GL_AREA(area), TRUE);
    gtk_gl_area_set_required_version(GTK_GL_AREA(area), 3, 0);
    gtk_container_add(GTK_CONTAINER(window), area);
    g_signal_connect(area, "render", G_CALLBACK(render), NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(window);
    g_timeout_add_seconds(20, finish, NULL);
    gtk_main();
    if (!rendered || gtk_gl_area_get_error(GTK_GL_AREA(area)))
    {
        const GError *error = gtk_gl_area_get_error(GTK_GL_AREA(area));
        fprintf(stderr, "GTK_WAYLAND_GPU_FAIL render=%u error=%s\n", rendered,
                error ? error->message : "no frame");
        result = 1;
    }
    gtk_widget_destroy(window);
    return result;
}
