/* SPDX-License-Identifier: BSD-3-Clause */
#include "config.h"
#include "libweston-internal.h"
#include "pixel-formats.h"
#include "shared/weston-drm-fourcc.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void lume_weston_render_check(struct weston_output *output, uint64_t frame)
{
    static bool checked;
    if (checked || frame < 10)
        return;
    const struct pixel_format_info *format = pixel_format_get_info(DRM_FORMAT_ABGR8888);
    const unsigned points[][2] = {{40, 40}, {161, 111}, {699, 599}};
    unsigned green = 0;
    for (unsigned i = 0; i < 3; ++i)
    {
        uint8_t pixel[4];
        unsigned x = points[i][0], y = points[i][1];
        if (output->compositor->renderer->read_pixels(output, format, pixel, x, y, 1, 1) ||
            abs((int)pixel[0] - (int)((x - 32) & 255)) > 2 || abs((int)pixel[2] - 64) > 2)
            goto fail;
        unsigned phase = (pixel[1] - (y - 32)) & 255;
        if (i && abs((int)phase - (int)green) > 2)
            goto fail;
        green = phase;
    }
    checked = true;
    puts("WAYLAND_RENDER_PASS samples=3");
    fflush(stdout);
    return;
fail:
    fputs("WAYLAND_RENDER_FAIL\n", stderr);
    fflush(stderr);
    exit(1);
}
