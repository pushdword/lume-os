/* SPDX-License-Identifier: BSD-3-Clause */
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gio/gio.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const unsigned char gif[] = {
    'G', 'I', 'F', '8', '9', 'a',  1, 0, 1, 0, 0x80, 0, 0, 0, 0, 0, 255, 255,  255, 0x21, 0xf9, 4,
    1,   0,   0,   0,   0,   0x2c, 0, 0, 0, 0, 1,    0, 1, 0, 0, 2, 2,   0x44, 1,   0,    0x3b};

int main(void)
{
    g_setenv("XDG_DATA_DIRS", "/usr/share", TRUE);
    gboolean uncertain = TRUE;
    gchar *type = g_content_type_guess("sample.c", NULL, 0, &uncertain);
    g_assert_cmpstr(type, ==, "text/x-csrc");
    g_assert(!uncertain);
    g_free(type);
    puts("PIXBUF_MIME_EXTENSION_PASS");

    GError *error = NULL;
    GdkPixbuf *image = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 3, 2);
    assert(image);
    gdk_pixbuf_fill(image, 0x257bcfff);
    gchar *encoded;
    gsize bytes;
    assert(gdk_pixbuf_save_to_buffer(image, &encoded, &bytes, "png", &error, NULL));
    g_assert_no_error(error);
    assert(bytes > 24 && !memcmp(encoded, "\x89PNG\r\n\x1a\n", 8));
    type = g_content_type_guess("unknown.bin", (const guchar *)encoded, bytes, &uncertain);
    g_assert_cmpstr(type, ==, "image/png");
    g_assert(!uncertain);
    g_free(type);
    const char *path = "/data/ação-imagem.bin";
    assert(g_file_set_contents(path, encoded, bytes, &error));
    g_assert_no_error(error);
    GdkPixbuf *decoded = gdk_pixbuf_new_from_file(path, &error);
    g_assert_no_error(error);
    assert(decoded && gdk_pixbuf_get_width(decoded) == 3 && gdk_pixbuf_get_height(decoded) == 2);
    assert(!memcmp(gdk_pixbuf_get_pixels(decoded), "\x25\x7b\xcf\xff", 4));
    GdkPixbuf *scaled = gdk_pixbuf_scale_simple(decoded, 12, 8, GDK_INTERP_BILINEAR);
    assert(scaled && gdk_pixbuf_get_width(scaled) == 12 && gdk_pixbuf_get_height(scaled) == 8);
    assert(!memcmp(gdk_pixbuf_get_pixels(scaled), "\x25\x7b\xcf\xff", 4));
    g_object_unref(scaled);
    g_object_unref(decoded);
    g_object_unref(image);
    g_free(encoded);
    assert(!unlink(path));
    puts("PIXBUF_PNG_MIME_SCALE_PASS");

    const char *xpm[] = {"2 1 2 1", "a c #257bcf", ". c None", "a."};
    image = gdk_pixbuf_new_from_xpm_data(xpm);
    assert(image && gdk_pixbuf_get_has_alpha(image));
    assert(!memcmp(gdk_pixbuf_get_pixels(image), "\x25\x7b\xcf\xff", 4));
    assert(gdk_pixbuf_get_pixels(image)[7] == 0);
    g_object_unref(image);
    GdkPixbufLoader *loader = gdk_pixbuf_loader_new_with_type("gif", &error);
    g_assert_no_error(error);
    assert(loader && gdk_pixbuf_loader_write(loader, gif, sizeof(gif), &error));
    g_assert_no_error(error);
    assert(gdk_pixbuf_loader_close(loader, &error));
    g_assert_no_error(error);
    image = gdk_pixbuf_loader_get_pixbuf(loader);
    assert(image && gdk_pixbuf_get_width(image) == 1 && gdk_pixbuf_get_height(image) == 1);
    g_object_unref(loader);
    puts("PIXBUF_XPM_GIF_PASS");
    puts("PIXBUF_NATIVE_PASS");
    return 0;
}
