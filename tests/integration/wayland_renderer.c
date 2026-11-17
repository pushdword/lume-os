/* SPDX-License-Identifier: BSD-3-Clause */
#include "config.h"
#include "libweston-internal.h"
#include "pixel-formats.h"
#include "shared/weston-drm-fourcc.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void lume_weston_render_check(struct weston_output *output, uint64_t frame, uint32_t storage_format)
{
    static bool checked;
    if (checked || frame < 10)
        return;
    const struct pixel_format_info *format = pixel_format_get_info(DRM_FORMAT_ABGR8888);
    const unsigned points[][2] = {{40, 40}, {161, 111}, {699, 599}};
    bool rgb565 = storage_format == DRM_FORMAT_RGB565 || storage_format == DRM_FORMAT_BGR565;
    unsigned color_tolerance = rgb565 ? 4 : 2;
    unsigned phase_tolerance = rgb565 ? 4 : 2;
    unsigned green = 0;
    for (unsigned i = 0; i < 3; ++i)
    {
        uint8_t pixel[4] = {0};
        unsigned x = points[i][0], y = points[i][1];
        int result = output->compositor->renderer->read_pixels(output, format, pixel, x, y, 1, 1);
        printf("WAYLAND_RENDER_SAMPLE x=%u y=%u rgba=%u,%u,%u,%u error=%d\n", x, y, pixel[0],
               pixel[1], pixel[2], pixel[3], result);
        if (result || (unsigned)abs((int)pixel[0] - (int)((x - 32) & 255)) > color_tolerance ||
            (unsigned)abs((int)pixel[2] - 64) > color_tolerance)
            goto fail;
        unsigned phase = (pixel[1] - (y - 32)) & 255;
        unsigned delta = (phase - green) & 255;
        if (i && delta > phase_tolerance && delta < 256 - phase_tolerance)
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
