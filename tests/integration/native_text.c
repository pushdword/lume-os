/* SPDX-License-Identifier: BSD-3-Clause */
#include <hb.h>
#include <hb-ft.h>
#include <hb-glib.h>
#include <hb-gobject.h>
#include <fribidi.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <glib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

static unsigned shape(hb_font_t *font, const char *text, hb_position_t *advance)
{
    hb_buffer_t *buffer = hb_buffer_create();
    assert(hb_buffer_allocation_successful(buffer));
    hb_buffer_add_utf8(buffer, text, -1, 0, -1);
    hb_buffer_guess_segment_properties(buffer);
    hb_shape(font, buffer, NULL, 0);
    unsigned count = 0;
    hb_glyph_info_t *glyphs = hb_buffer_get_glyph_infos(buffer, &count);
    hb_glyph_position_t *positions = hb_buffer_get_glyph_positions(buffer, NULL);
    assert(count && glyphs && positions);
    *advance = 0;
    for (unsigned i = 0; i < count; ++i)
    {
        assert(glyphs[i].codepoint);
        *advance += positions[i].x_advance;
    }
    hb_buffer_destroy(buffer);
    return count;
}

static gpointer concurrent_text(gpointer argument)
{
    const char enter[] = "TEXT_THREAD_ENTER\n";
    assert(write(STDOUT_FILENO, enter, sizeof(enter) - 1) == sizeof(enter) - 1);
    FT_Library library;
    FT_Face face;
    assert(!FT_Init_FreeType(&library));
    const char initialized[] = "TEXT_THREAD_LIBRARY_READY\n";
    assert(write(STDOUT_FILENO, initialized, sizeof(initialized) - 1) == sizeof(initialized) - 1);
    assert(!FT_New_Face(library, "/usr/share/fonts/dejavu/DejaVuSans.ttf", 0, &face));
    assert(!FT_Set_Pixel_Sizes(face, 0, 18));
    const char ready[] = "TEXT_THREAD_FONT_READY\n";
    assert(write(STDOUT_FILENO, ready, sizeof(ready) - 1) == sizeof(ready) - 1);
    hb_font_t *font = hb_ft_font_create_referenced(face);
    for (unsigned i = 0; i < 100; ++i)
    {
        hb_variation_t variation;
        assert(hb_variation_from_string("wght=532.5", -1, &variation));
        assert(variation.value == 532.5f);
        hb_position_t advance;
        assert(shape(font, "ação", &advance) == 4 && advance > 0);
    }
    hb_font_destroy(font);
    assert(!FT_Done_Face(face));
    assert(!FT_Done_FreeType(library));
    return argument;
}

int main(int argc, char **argv)
{
    puts("TEXT_CONCURRENT_START");
    fflush(stdout);
    GThread *workers[4];
    unsigned count = argc == 2 ? (unsigned)strtoul(argv[1], NULL, 10) : G_N_ELEMENTS(workers);
    assert(count > 0 && count <= G_N_ELEMENTS(workers));
    for (unsigned i = 0; i < count; ++i)
        workers[i] = g_thread_new("text", concurrent_text, GUINT_TO_POINTER(i + 1));
    for (unsigned i = 0; i < count; ++i)
        assert(g_thread_join(workers[i]) == GUINT_TO_POINTER(i + 1));
    puts("TEXT_CONCURRENT_SHAPING_PASS");
    hb_variation_t variation;
    assert(hb_variation_from_string("wght=532.5", -1, &variation));
    assert(variation.tag == HB_TAG('w', 'g', 'h', 't') && variation.value == 532.5f);
    assert(hb_unicode_combining_class(hb_glib_get_unicode_funcs(), 0x0301) ==
           HB_UNICODE_COMBINING_CLASS_ABOVE);
    assert(g_type_is_a(HB_GOBJECT_TYPE_BUFFER, G_TYPE_BOXED));
    puts("TEXT_LOCALE_UNICODE_PASS");

    const FriBidiChar logical[] = {'A', ' ', 0x05d0, 0x05d1, 0x05d2};
    FriBidiChar visual[G_N_ELEMENTS(logical)];
    FriBidiParType direction = FRIBIDI_PAR_LTR;
    assert(fribidi_log2vis(logical, G_N_ELEMENTS(logical), &direction, visual, NULL, NULL, NULL));
    assert(visual[0] == 'A' && visual[1] == ' ');
    assert(visual[2] == 0x05d2 && visual[3] == 0x05d1 && visual[4] == 0x05d0);
    puts("TEXT_BIDI_PASS");

    FT_Library library;
    FT_Face face;
    assert(!FT_Init_FreeType(&library));
    assert(!FT_New_Face(library, "/usr/share/fonts/dejavu/DejaVuSans.ttf", 0, &face));
    assert(!strcmp(face->family_name, "DejaVu Sans"));
    for (unsigned size = 12; size <= 24; size += 6)
    {
        assert(!FT_Set_Pixel_Sizes(face, 0, size));
        hb_font_t *font = hb_ft_font_create_referenced(face);
        assert(font != hb_font_get_empty());
        hb_position_t normal, decomposed;
        assert(shape(font, "ação", &normal) == 4 && normal > 0);
        assert(shape(font,
                     "ac\xcc\xa7"
                     "a\xcc\x83"
                     "o",
                     &decomposed) == 4);
        assert(normal == decomposed);
        assert(!FT_Load_Char(face, 0x00e7, FT_LOAD_RENDER));
        assert(face->glyph->bitmap.width && face->glyph->bitmap.rows);
        assert(face->glyph->bitmap.buffer);
        hb_font_destroy(font);
    }
    assert(!FT_Done_Face(face));
    assert(!FT_Done_FreeType(library));
    puts("TEXT_FONT_RASTER_PASS");
    puts("TEXT_NATIVE_PASS");
    return 0;
}
