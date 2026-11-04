/* SPDX-License-Identifier: BSD-3-Clause */
#include <cairo.h>
#include <cairo-ft.h>
#include <fontconfig/fontconfig.h>
#include <expat.h>
#include <pango/pangocairo.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void element(void *argument, const XML_Char *name, const XML_Char **attributes)
{
    unsigned *count = argument;
    assert(!strcmp(name, "fonts") || !strcmp(name, "font"));
    if (!strcmp(name, "font"))
        assert(!strcmp(attributes[0], "name") && !strcmp(attributes[1], "ação"));
    ++*count;
}

static FcPattern *match(const char *family)
{
    FcPattern *request = FcPatternCreate();
    assert(request && FcPatternAddString(request, FC_FAMILY, (const FcChar8 *)family));
    assert(FcConfigSubstitute(NULL, request, FcMatchPattern));
    FcDefaultSubstitute(request);
    FcResult result;
    FcPattern *font = FcFontMatch(NULL, request, &result);
    FcPatternDestroy(request);
    assert(font && result == FcResultMatch);
    return font;
}

int main(void)
{
    const char xml[] = "<fonts><font name=\"ação\"/></fonts>";
    XML_Parser parser = XML_ParserCreate("UTF-8");
    assert(parser);
    unsigned elements = 0;
    XML_SetUserData(parser, &elements);
    XML_SetStartElementHandler(parser, element);
    assert(XML_Parse(parser, xml, sizeof(xml) - 1, XML_TRUE) == XML_STATUS_OK);
    assert(elements == 2);
    XML_ParserFree(parser);
    puts("RENDER_XML_PASS");

    assert(FcInit());
    FcPattern *mono = match("monospace");
    FcChar8 *path;
    assert(FcPatternGetString(mono, FC_FILE, 0, &path) == FcResultMatch);
    assert(strstr((char *)path, "DejaVuSansMono.ttf"));
    FcPatternDestroy(mono);
    FcPattern *font = match("sans-serif");
    FcCharSet *charset;
    assert(FcPatternGetCharSet(font, FC_CHARSET, 0, &charset) == FcResultMatch);
    assert(FcCharSetHasChar(charset, 0x00e7) && FcCharSetHasChar(charset, 0x00e3));
    puts("RENDER_FONT_SELECTION_PASS");

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 240, 80);
    assert(cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS);
    cairo_t *context = cairo_create(surface);
    cairo_set_source_rgb(context, 1, 1, 1);
    cairo_paint(context);
    cairo_set_source_rgb(context, 0, 0, 1);
    cairo_rectangle(context, 0, 0, 16, 16);
    cairo_fill(context);
    cairo_font_face_t *face = cairo_ft_font_face_create_for_pattern(font);
    assert(cairo_font_face_status(face) == CAIRO_STATUS_SUCCESS);
    cairo_set_font_face(context, face);
    cairo_set_font_size(context, 24);
    cairo_text_extents_t extents;
    cairo_text_extents(context, "ação", &extents);
    assert(extents.width > 35 && extents.width < 120 && extents.height > 16);
    cairo_set_source_rgb(context, 0, 0, 0);
    cairo_move_to(context, 30, 45);
    cairo_show_text(context, "ação");
    PangoLayout *layout = pango_cairo_create_layout(context);
    PangoFontDescription *description = pango_font_description_from_string("DejaVu Sans 14");
    pango_layout_set_font_description(layout, description);
    pango_font_description_free(description);
    pango_layout_set_width(layout, 120 * PANGO_SCALE);
    pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
    pango_layout_set_text(layout, "ação, café e informação em português", -1);
    assert(pango_layout_get_unknown_glyphs_count(layout) == 0);
    assert(pango_layout_get_line_count(layout) >= 2);
    int width, height;
    pango_layout_get_pixel_size(layout, &width, &height);
    assert(width > 50 && width <= 120 && height > 25);
    cairo_move_to(context, 110, 0);
    pango_cairo_show_layout(context, layout);
    g_object_unref(layout);
    puts("RENDER_PANGO_LAYOUT_PASS");
    assert(cairo_status(context) == CAIRO_STATUS_SUCCESS);
    cairo_surface_flush(surface);
    uint32_t *pixels = (uint32_t *)cairo_image_surface_get_data(surface);
    assert(pixels[0] == UINT32_C(0xff0000ff));
    assert(pixels[20] == UINT32_C(0xffffffff));
    unsigned dark = 0;
    int stride = cairo_image_surface_get_stride(surface) / sizeof(*pixels);
    for (unsigned y = 20; y < 48; ++y)
        for (unsigned x = 28; x < 150; ++x)
            if ((pixels[y * stride + x] & 0x00ffffff) != 0x00ffffff)
                ++dark;
    assert(dark > 100);
    const char *png = "/data/render-check.png";
    assert(cairo_surface_write_to_png(surface, png) == CAIRO_STATUS_SUCCESS);
    FILE *file = fopen(png, "rb");
    unsigned char signature[8];
    assert(file && fread(signature, 1, sizeof(signature), file) == sizeof(signature));
    assert(!memcmp(signature, "\x89PNG\r\n\x1a\n", sizeof(signature)));
    assert(!fclose(file) && !unlink(png));
    cairo_destroy(context);
    cairo_surface_destroy(surface);
    cairo_font_face_destroy(face);
    FcPatternDestroy(font);
    pango_cairo_font_map_set_default(NULL);
    cairo_debug_reset_static_data();
    FcFini();
    puts("RENDER_CAIRO_PNG_PASS");
    puts("RENDER_NATIVE_PASS");
    return 0;
}
