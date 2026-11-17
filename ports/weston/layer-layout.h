/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_WESTON_LAYER_LAYOUT_H
#define LUME_WESTON_LAYER_LAYOUT_H
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>

enum lume_layer_anchor
{
    LUME_ANCHOR_TOP = 1,
    LUME_ANCHOR_BOTTOM = 2,
    LUME_ANCHOR_LEFT = 4,
    LUME_ANCHOR_RIGHT = 8
};

struct lume_layer_rect
{
    int32_t x, y, width, height;
};

struct lume_layer_state
{
    uint32_t width, height, anchor, layer, keyboard;
    int32_t top, right, bottom, left, exclusive;
};

static inline int32_t lume_layer_clamp(int64_t value)
{
    return value < INT32_MIN ? INT32_MIN : value > INT32_MAX ? INT32_MAX : (int32_t)value;
}

static inline bool lume_layer_valid(const struct lume_layer_state *state)
{
    unsigned horizontal = LUME_ANCHOR_LEFT | LUME_ANCHOR_RIGHT;
    unsigned vertical = LUME_ANCHOR_TOP | LUME_ANCHOR_BOTTOM;
    return !(state->anchor & ~15u) && state->layer < 4 && state->keyboard < 3 &&
           state->width <= INT32_MAX && state->height <= INT32_MAX &&
           (state->width || (state->anchor & horizontal) == horizontal) &&
           (state->height || (state->anchor & vertical) == vertical);
}

static inline struct lume_layer_rect lume_layer_place(const struct lume_layer_state *state,
                                                      struct lume_layer_rect area)
{
    int64_t available_width = (int64_t)area.width - state->left - state->right;
    int64_t available_height = (int64_t)area.height - state->top - state->bottom;
    int64_t width = state->width ? state->width : available_width;
    int64_t height = state->height ? state->height : available_height;
    width = width < 1 ? 1 : width;
    height = height < 1 ? 1 : height;
    int64_t x = (int64_t)area.x + state->left + (available_width - width) / 2;
    int64_t y = (int64_t)area.y + state->top + (available_height - height) / 2;
    if ((state->anchor & (LUME_ANCHOR_LEFT | LUME_ANCHOR_RIGHT)) == LUME_ANCHOR_LEFT)
        x = (int64_t)area.x + state->left;
    else if ((state->anchor & (LUME_ANCHOR_LEFT | LUME_ANCHOR_RIGHT)) == LUME_ANCHOR_RIGHT)
        x = (int64_t)area.x + area.width - width - state->right;
    if ((state->anchor & (LUME_ANCHOR_TOP | LUME_ANCHOR_BOTTOM)) == LUME_ANCHOR_TOP)
        y = (int64_t)area.y + state->top;
    else if ((state->anchor & (LUME_ANCHOR_TOP | LUME_ANCHOR_BOTTOM)) == LUME_ANCHOR_BOTTOM)
        y = (int64_t)area.y + area.height - height - state->bottom;
    return (struct lume_layer_rect){lume_layer_clamp(x), lume_layer_clamp(y),
                                    lume_layer_clamp(width), lume_layer_clamp(height)};
}

static inline void lume_layer_reserve(const struct lume_layer_state *state,
                                      struct lume_layer_rect *area)
{
    if (state->exclusive <= 0)
        return;
    uint32_t anchors[] = {LUME_ANCHOR_TOP, LUME_ANCHOR_RIGHT, LUME_ANCHOR_BOTTOM, LUME_ANCHOR_LEFT};
    uint32_t parallel[] = {
        LUME_ANCHOR_LEFT | LUME_ANCHOR_RIGHT, LUME_ANCHOR_TOP | LUME_ANCHOR_BOTTOM,
        LUME_ANCHOR_LEFT | LUME_ANCHOR_RIGHT, LUME_ANCHOR_TOP | LUME_ANCHOR_BOTTOM};
    int32_t margins[] = {state->top, state->right, state->bottom, state->left};
    for (unsigned i = 0; i < 4; ++i)
        if (state->anchor == anchors[i] || state->anchor == (anchors[i] | parallel[i]))
        {
            int64_t amount = (int64_t)state->exclusive + margins[i];
            int32_t limit = (i & 1 ? area->width : area->height) - 1;
            int32_t reserved = amount <= 0 ? 0 : amount > limit ? limit : (int32_t)amount;
            if (i & 1)
            {
                area->width -= reserved;
                if (i == 3)
                    area->x += reserved;
            }
            else
            {
                area->height -= reserved;
                if (!i)
                    area->y += reserved;
            }
            return;
        }
}
#endif
