/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_XORG_GLAMOR_H
#define LUME_XORG_GLAMOR_H

Bool lume_glamor_prepare(int input, unsigned *width, unsigned *height);
Bool lume_glamor_screen_init(ScreenPtr screen);
Bool lume_glamor_screen_resources(ScreenPtr screen);
Bool lume_glamor_present(ScreenPtr screen);

#endif
