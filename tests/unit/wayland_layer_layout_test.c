/* SPDX-License-Identifier: BSD-3-Clause */
#include "../../ports/weston/layer-layout.h"
#include <assert.h>
#include <stdio.h>

static void rectangle(struct lume_layer_rect actual, int x, int y, int width, int height)
{
    assert(actual.x == x && actual.y == y);
    assert(actual.width == width && actual.height == height);
}

int main(void)
{
    struct lume_layer_rect area = {0, 0, 1920, 1080};
    struct lume_layer_state desktop = {.anchor = 15, .exclusive = -1};
    assert(lume_layer_valid(&desktop));
    rectangle(lume_layer_place(&desktop, area), 0, 0, 1920, 1080);
    struct lume_layer_state panel = {.height = 32, .anchor = 13, .exclusive = 32};
    assert(lume_layer_valid(&panel));
    rectangle(lume_layer_place(&panel, area), 0, 0, 1920, 32);
    lume_layer_reserve(&panel, &area);
    rectangle(area, 0, 32, 1920, 1048);
    rectangle(lume_layer_place(&desktop, area), 0, 32, 1920, 1048);
    panel.anchor = 14;
    panel.bottom = 8;
    rectangle(lume_layer_place(&panel, area), 0, 1040, 1920, 32);
    lume_layer_reserve(&panel, &area);
    rectangle(area, 0, 32, 1920, 1008);
    struct lume_layer_state dock = {.width = 48, .anchor = 7, .exclusive = 48};
    lume_layer_reserve(&dock, &area);
    rectangle(area, 48, 32, 1872, 1008);
    dock.anchor = 11;
    dock.right = 12;
    lume_layer_reserve(&dock, &area);
    rectangle(area, 48, 32, 1812, 1008);
    panel.width = 400;
    panel.left = 20;
    panel.right = 4;
    rectangle(lume_layer_place(&panel, (struct lume_layer_rect){0, 0, 1920, 1080}), 768, 1040, 400,
              32);
    panel.anchor = 5;
    area = (struct lume_layer_rect){0, 0, 100, 100};
    lume_layer_reserve(&panel, &area);
    rectangle(area, 0, 0, 100, 100);
    panel.anchor = 13;
    panel.width = 0;
    panel.left = INT32_MIN;
    panel.right = INT32_MIN;
    panel.top = INT32_MAX;
    panel.exclusive = INT32_MAX;
    struct lume_layer_rect large = lume_layer_place(&panel, area);
    assert(large.width == INT32_MAX && large.height == 32);
    lume_layer_reserve(&panel, &area);
    rectangle(area, 0, 99, 100, 1);
    panel.width = UINT32_MAX;
    assert(!lume_layer_valid(&panel));
    panel.width = 0;
    panel.anchor = LUME_ANCHOR_TOP;
    assert(!lume_layer_valid(&panel));
    panel.anchor = 13;
    panel.keyboard = 3;
    assert(!lume_layer_valid(&panel));
    puts("WAYLAND_LAYER_LAYOUT_PASS");
    return 0;
}
