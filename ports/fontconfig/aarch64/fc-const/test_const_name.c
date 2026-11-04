/* Copyright (C) 2025 fontconfig Authors */
/* SPDX-License-Identifier: HPND */

#include <stdio.h>
#include "fontconfig/fontconfig.h"

int test (void) {
    int ret = 0;
    const FcConstant *c;
    c = FcNameGetConstantFor ((const FcChar8 *)"antialias", FC_ANTIALIAS);
    if (!c || c->value != FcTrue) {
        fprintf (stderr, "failed: (%s, %s)\n", "antialias", _FC_STRINGIFY(FC_ANTIALIAS));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"autohint", FC_AUTOHINT);
    if (!c || c->value != FcTrue) {
        fprintf (stderr, "failed: (%s, %s)\n", "autohint", _FC_STRINGIFY(FC_AUTOHINT));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"bgr", FC_RGBA);
    if (!c || c->value != FC_RGBA_BGR) {
        fprintf (stderr, "failed: (%s, %s)\n", "bgr", _FC_STRINGIFY(FC_RGBA));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"black", FC_WEIGHT);
    if (!c || c->value != FC_WEIGHT_BLACK) {
        fprintf (stderr, "failed: (%s, %s)\n", "black", _FC_STRINGIFY(FC_WEIGHT));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"bold", FC_WEIGHT);
    if (!c || c->value != FC_WEIGHT_BOLD) {
        fprintf (stderr, "failed: (%s, %s)\n", "bold", _FC_STRINGIFY(FC_WEIGHT));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"book", FC_WEIGHT);
    if (!c || c->value != FC_WEIGHT_BOOK) {
        fprintf (stderr, "failed: (%s, %s)\n", "book", _FC_STRINGIFY(FC_WEIGHT));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"charcell", FC_SPACING);
    if (!c || c->value != FC_SPACING_CHARCELL) {
        fprintf (stderr, "failed: (%s, %s)\n", "charcell", _FC_STRINGIFY(FC_SPACING));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"condensed", FC_WIDTH);
    if (!c || c->value != FC_WIDTH_CONDENSED) {
        fprintf (stderr, "failed: (%s, %s)\n", "condensed", _FC_STRINGIFY(FC_WIDTH));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"cursive", FC_GENERIC_FAMILY);
    if (!c || c->value != FC_FAMILY_CURSIVE) {
        fprintf (stderr, "failed: (%s, %s)\n", "cursive", _FC_STRINGIFY(FC_GENERIC_FAMILY));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"decorative", FC_DECORATIVE);
    if (!c || c->value != FcTrue) {
        fprintf (stderr, "failed: (%s, %s)\n", "decorative", _FC_STRINGIFY(FC_DECORATIVE));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"demibold", FC_WEIGHT);
    if (!c || c->value != FC_WEIGHT_DEMIBOLD) {
        fprintf (stderr, "failed: (%s, %s)\n", "demibold", _FC_STRINGIFY(FC_WEIGHT));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"demilight", FC_WEIGHT);
    if (!c || c->value != FC_WEIGHT_DEMILIGHT) {
        fprintf (stderr, "failed: (%s, %s)\n", "demilight", _FC_STRINGIFY(FC_WEIGHT));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"dual", FC_SPACING);
    if (!c || c->value != FC_SPACING_DUAL) {
        fprintf (stderr, "failed: (%s, %s)\n", "dual", _FC_STRINGIFY(FC_SPACING));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"embeddedbitmap", FC_EMBEDDED_BITMAP);
    if (!c || c->value != FcTrue) {
        fprintf (stderr, "failed: (%s, %s)\n", "embeddedbitmap", _FC_STRINGIFY(FC_EMBEDDED_BITMAP));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"embolden", FC_EMBOLDEN);
    if (!c || c->value != FcTrue) {
        fprintf (stderr, "failed: (%s, %s)\n", "embolden", _FC_STRINGIFY(FC_EMBOLDEN));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"emoji", FC_GENERIC_FAMILY);
    if (!c || c->value != FC_FAMILY_EMOJI) {
        fprintf (stderr, "failed: (%s, %s)\n", "emoji", _FC_STRINGIFY(FC_GENERIC_FAMILY));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"expanded", FC_WIDTH);
    if (!c || c->value != FC_WIDTH_EXPANDED) {
        fprintf (stderr, "failed: (%s, %s)\n", "expanded", _FC_STRINGIFY(FC_WIDTH));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"extrablack", FC_WEIGHT);
    if (!c || c->value != FC_WEIGHT_EXTRABLACK) {
        fprintf (stderr, "failed: (%s, %s)\n", "extrablack", _FC_STRINGIFY(FC_WEIGHT));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"extrabold", FC_WEIGHT);
    if (!c || c->value != FC_WEIGHT_EXTRABOLD) {
        fprintf (stderr, "failed: (%s, %s)\n", "extrabold", _FC_STRINGIFY(FC_WEIGHT));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"extracondensed", FC_WIDTH);
    if (!c || c->value != FC_WIDTH_EXTRACONDENSED) {
        fprintf (stderr, "failed: (%s, %s)\n", "extracondensed", _FC_STRINGIFY(FC_WIDTH));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"extraexpanded", FC_WIDTH);
    if (!c || c->value != FC_WIDTH_EXTRAEXPANDED) {
        fprintf (stderr, "failed: (%s, %s)\n", "extraexpanded", _FC_STRINGIFY(FC_WIDTH));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"extralight", FC_WEIGHT);
    if (!c || c->value != FC_WEIGHT_EXTRALIGHT) {
        fprintf (stderr, "failed: (%s, %s)\n", "extralight", _FC_STRINGIFY(FC_WEIGHT));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"fangsong", FC_GENERIC_FAMILY);
    if (!c || c->value != FC_FAMILY_FANGSONG) {
        fprintf (stderr, "failed: (%s, %s)\n", "fangsong", _FC_STRINGIFY(FC_GENERIC_FAMILY));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"fantasy", FC_GENERIC_FAMILY);
    if (!c || c->value != FC_FAMILY_FANTASY) {
        fprintf (stderr, "failed: (%s, %s)\n", "fantasy", _FC_STRINGIFY(FC_GENERIC_FAMILY));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"globaladvance", FC_GLOBAL_ADVANCE);
    if (!c || c->value != FcTrue) {
        fprintf (stderr, "failed: (%s, %s)\n", "globaladvance", _FC_STRINGIFY(FC_GLOBAL_ADVANCE));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"heavy", FC_WEIGHT);
    if (!c || c->value != FC_WEIGHT_HEAVY) {
        fprintf (stderr, "failed: (%s, %s)\n", "heavy", _FC_STRINGIFY(FC_WEIGHT));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"hintfull", FC_HINT_STYLE);
    if (!c || c->value != FC_HINT_FULL) {
        fprintf (stderr, "failed: (%s, %s)\n", "hintfull", _FC_STRINGIFY(FC_HINT_STYLE));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"hinting", FC_HINTING);
    if (!c || c->value != FcTrue) {
        fprintf (stderr, "failed: (%s, %s)\n", "hinting", _FC_STRINGIFY(FC_HINTING));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"hintmedium", FC_HINT_STYLE);
    if (!c || c->value != FC_HINT_MEDIUM) {
        fprintf (stderr, "failed: (%s, %s)\n", "hintmedium", _FC_STRINGIFY(FC_HINT_STYLE));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"hintnone", FC_HINT_STYLE);
    if (!c || c->value != FC_HINT_NONE) {
        fprintf (stderr, "failed: (%s, %s)\n", "hintnone", _FC_STRINGIFY(FC_HINT_STYLE));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"hintslight", FC_HINT_STYLE);
    if (!c || c->value != FC_HINT_SLIGHT) {
        fprintf (stderr, "failed: (%s, %s)\n", "hintslight", _FC_STRINGIFY(FC_HINT_STYLE));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"italic", FC_SLANT);
    if (!c || c->value != FC_SLANT_ITALIC) {
        fprintf (stderr, "failed: (%s, %s)\n", "italic", _FC_STRINGIFY(FC_SLANT));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"lcddefault", FC_LCD_FILTER);
    if (!c || c->value != FC_LCD_DEFAULT) {
        fprintf (stderr, "failed: (%s, %s)\n", "lcddefault", _FC_STRINGIFY(FC_LCD_FILTER));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"lcdlegacy", FC_LCD_FILTER);
    if (!c || c->value != FC_LCD_LEGACY) {
        fprintf (stderr, "failed: (%s, %s)\n", "lcdlegacy", _FC_STRINGIFY(FC_LCD_FILTER));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"lcdlight", FC_LCD_FILTER);
    if (!c || c->value != FC_LCD_LIGHT) {
        fprintf (stderr, "failed: (%s, %s)\n", "lcdlight", _FC_STRINGIFY(FC_LCD_FILTER));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"lcdnone", FC_LCD_FILTER);
    if (!c || c->value != FC_LCD_NONE) {
        fprintf (stderr, "failed: (%s, %s)\n", "lcdnone", _FC_STRINGIFY(FC_LCD_FILTER));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"light", FC_WEIGHT);
    if (!c || c->value != FC_WEIGHT_LIGHT) {
        fprintf (stderr, "failed: (%s, %s)\n", "light", _FC_STRINGIFY(FC_WEIGHT));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"math", FC_GENERIC_FAMILY);
    if (!c || c->value != FC_FAMILY_MATH) {
        fprintf (stderr, "failed: (%s, %s)\n", "math", _FC_STRINGIFY(FC_GENERIC_FAMILY));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"medium", FC_WEIGHT);
    if (!c || c->value != FC_WEIGHT_MEDIUM) {
        fprintf (stderr, "failed: (%s, %s)\n", "medium", _FC_STRINGIFY(FC_WEIGHT));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"minspace", FC_MINSPACE);
    if (!c || c->value != FcTrue) {
        fprintf (stderr, "failed: (%s, %s)\n", "minspace", _FC_STRINGIFY(FC_MINSPACE));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"mono", FC_SPACING);
    if (!c || c->value != FC_SPACING_MONO) {
        fprintf (stderr, "failed: (%s, %s)\n", "mono", _FC_STRINGIFY(FC_SPACING));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"monospace", FC_GENERIC_FAMILY);
    if (!c || c->value != FC_FAMILY_MONO) {
        fprintf (stderr, "failed: (%s, %s)\n", "monospace", _FC_STRINGIFY(FC_GENERIC_FAMILY));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"none", FC_RGBA);
    if (!c || c->value != FC_RGBA_NONE) {
        fprintf (stderr, "failed: (%s, %s)\n", "none", _FC_STRINGIFY(FC_RGBA));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"normal", FC_WEIGHT);
    if (!c || c->value != FC_WEIGHT_NORMAL) {
        fprintf (stderr, "failed: (%s, %s)\n", "normal", _FC_STRINGIFY(FC_WEIGHT));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"normal", FC_WIDTH);
    if (!c || c->value != FC_WIDTH_NORMAL) {
        fprintf (stderr, "failed: (%s, %s)\n", "normal", _FC_STRINGIFY(FC_WIDTH));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"oblique", FC_SLANT);
    if (!c || c->value != FC_SLANT_OBLIQUE) {
        fprintf (stderr, "failed: (%s, %s)\n", "oblique", _FC_STRINGIFY(FC_SLANT));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"outline", FC_OUTLINE);
    if (!c || c->value != FcTrue) {
        fprintf (stderr, "failed: (%s, %s)\n", "outline", _FC_STRINGIFY(FC_OUTLINE));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"proportional", FC_SPACING);
    if (!c || c->value != FC_SPACING_PROPORTIONAL) {
        fprintf (stderr, "failed: (%s, %s)\n", "proportional", _FC_STRINGIFY(FC_SPACING));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"regular", FC_WEIGHT);
    if (!c || c->value != FC_WEIGHT_REGULAR) {
        fprintf (stderr, "failed: (%s, %s)\n", "regular", _FC_STRINGIFY(FC_WEIGHT));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"rgb", FC_RGBA);
    if (!c || c->value != FC_RGBA_RGB) {
        fprintf (stderr, "failed: (%s, %s)\n", "rgb", _FC_STRINGIFY(FC_RGBA));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"roman", FC_SLANT);
    if (!c || c->value != FC_SLANT_ROMAN) {
        fprintf (stderr, "failed: (%s, %s)\n", "roman", _FC_STRINGIFY(FC_SLANT));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"sans-serif", FC_GENERIC_FAMILY);
    if (!c || c->value != FC_FAMILY_SANS) {
        fprintf (stderr, "failed: (%s, %s)\n", "sans-serif", _FC_STRINGIFY(FC_GENERIC_FAMILY));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"scalable", FC_SCALABLE);
    if (!c || c->value != FcTrue) {
        fprintf (stderr, "failed: (%s, %s)\n", "scalable", _FC_STRINGIFY(FC_SCALABLE));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"semibold", FC_WEIGHT);
    if (!c || c->value != FC_WEIGHT_DEMIBOLD) {
        fprintf (stderr, "failed: (%s, %s)\n", "semibold", _FC_STRINGIFY(FC_WEIGHT));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"semicondensed", FC_WIDTH);
    if (!c || c->value != FC_WIDTH_SEMICONDENSED) {
        fprintf (stderr, "failed: (%s, %s)\n", "semicondensed", _FC_STRINGIFY(FC_WIDTH));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"semiexpanded", FC_WIDTH);
    if (!c || c->value != FC_WIDTH_SEMIEXPANDED) {
        fprintf (stderr, "failed: (%s, %s)\n", "semiexpanded", _FC_STRINGIFY(FC_WIDTH));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"semilight", FC_WEIGHT);
    if (!c || c->value != FC_WEIGHT_DEMILIGHT) {
        fprintf (stderr, "failed: (%s, %s)\n", "semilight", _FC_STRINGIFY(FC_WEIGHT));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"serif", FC_GENERIC_FAMILY);
    if (!c || c->value != FC_FAMILY_SERIF) {
        fprintf (stderr, "failed: (%s, %s)\n", "serif", _FC_STRINGIFY(FC_GENERIC_FAMILY));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"system-ui", FC_GENERIC_FAMILY);
    if (!c || c->value != FC_FAMILY_SYSTEM_UI) {
        fprintf (stderr, "failed: (%s, %s)\n", "system-ui", _FC_STRINGIFY(FC_GENERIC_FAMILY));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"thin", FC_WEIGHT);
    if (!c || c->value != FC_WEIGHT_THIN) {
        fprintf (stderr, "failed: (%s, %s)\n", "thin", _FC_STRINGIFY(FC_WEIGHT));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"ui-monospace", FC_GENERIC_FAMILY);
    if (!c || c->value != FC_FAMILY_UI_MONO) {
        fprintf (stderr, "failed: (%s, %s)\n", "ui-monospace", _FC_STRINGIFY(FC_GENERIC_FAMILY));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"ui-rounded", FC_GENERIC_FAMILY);
    if (!c || c->value != FC_FAMILY_UI_ROUNDED) {
        fprintf (stderr, "failed: (%s, %s)\n", "ui-rounded", _FC_STRINGIFY(FC_GENERIC_FAMILY));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"ui-sans-serif", FC_GENERIC_FAMILY);
    if (!c || c->value != FC_FAMILY_UI_SANS) {
        fprintf (stderr, "failed: (%s, %s)\n", "ui-sans-serif", _FC_STRINGIFY(FC_GENERIC_FAMILY));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"ui-serif", FC_GENERIC_FAMILY);
    if (!c || c->value != FC_FAMILY_UI_SERIF) {
        fprintf (stderr, "failed: (%s, %s)\n", "ui-serif", _FC_STRINGIFY(FC_GENERIC_FAMILY));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"ultrablack", FC_WEIGHT);
    if (!c || c->value != FC_WEIGHT_ULTRABLACK) {
        fprintf (stderr, "failed: (%s, %s)\n", "ultrablack", _FC_STRINGIFY(FC_WEIGHT));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"ultrabold", FC_WEIGHT);
    if (!c || c->value != FC_WEIGHT_EXTRABOLD) {
        fprintf (stderr, "failed: (%s, %s)\n", "ultrabold", _FC_STRINGIFY(FC_WEIGHT));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"ultracondensed", FC_WIDTH);
    if (!c || c->value != FC_WIDTH_ULTRACONDENSED) {
        fprintf (stderr, "failed: (%s, %s)\n", "ultracondensed", _FC_STRINGIFY(FC_WIDTH));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"ultraexpanded", FC_WIDTH);
    if (!c || c->value != FC_WIDTH_ULTRAEXPANDED) {
        fprintf (stderr, "failed: (%s, %s)\n", "ultraexpanded", _FC_STRINGIFY(FC_WIDTH));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"ultralight", FC_WEIGHT);
    if (!c || c->value != FC_WEIGHT_EXTRALIGHT) {
        fprintf (stderr, "failed: (%s, %s)\n", "ultralight", _FC_STRINGIFY(FC_WEIGHT));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"unknown", FC_RGBA);
    if (!c || c->value != FC_RGBA_UNKNOWN) {
        fprintf (stderr, "failed: (%s, %s)\n", "unknown", _FC_STRINGIFY(FC_RGBA));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"vbgr", FC_RGBA);
    if (!c || c->value != FC_RGBA_VBGR) {
        fprintf (stderr, "failed: (%s, %s)\n", "vbgr", _FC_STRINGIFY(FC_RGBA));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"verticallayout", FC_VERTICAL_LAYOUT);
    if (!c || c->value != FcTrue) {
        fprintf (stderr, "failed: (%s, %s)\n", "verticallayout", _FC_STRINGIFY(FC_VERTICAL_LAYOUT));
        ret++;
    }
    c = FcNameGetConstantFor ((const FcChar8 *)"vrgb", FC_RGBA);
    if (!c || c->value != FC_RGBA_VRGB) {
        fprintf (stderr, "failed: (%s, %s)\n", "vrgb", _FC_STRINGIFY(FC_RGBA));
        ret++;
    }
    return ret;
}

int main(void) { return test(); }
