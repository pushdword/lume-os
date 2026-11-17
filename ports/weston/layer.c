/* SPDX-License-Identifier: BSD-3-Clause */
#include "layer.h"
#include "popup.h"
#include "wlr-layer-shell-unstable-v1-server-protocol.h"
#include <linux/input.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct lume_layer_shell
{
    struct weston_compositor *compositor;
    struct weston_output *output;
    struct weston_seat *seat;
    struct weston_layer layers[4];
    struct wl_list surfaces;
    struct wl_global *global;
    struct lume_layer_rect work_area;
    void (*changed)(void *);
    void *data;
};

struct layer_configure
{
    struct wl_list link;
    uint32_t serial;
};

struct layer_surface
{
    struct lume_layer_shell *shell;
    struct wl_resource *resource;
    struct weston_surface *surface;
    struct weston_view *view;
    struct wl_listener destroyed;
    struct wl_list link, configures;
    struct lume_layer_state pending, current;
    struct lume_layer_rect bounds;
    uint32_t configured_width, configured_height;
    unsigned configure_count;
    bool initialized, acknowledged;
    char name[96];
};

_Static_assert(LUME_ANCHOR_TOP == ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP, "Top anchor ABI");
_Static_assert(LUME_ANCHOR_RIGHT == ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT, "Right anchor ABI");

static struct lume_layer_rect full_area(struct lume_layer_shell *shell)
{
    return (struct lume_layer_rect){shell->output->pos.c.x, shell->output->pos.c.y,
                                    shell->output->width, shell->output->height};
}

static void clear_configures(struct layer_surface *surface)
{
    struct layer_configure *item, *next;
    wl_list_for_each_safe(item, next, &surface->configures, link)
    {
        wl_list_remove(&item->link);
        free(item);
    }
    surface->configure_count = 0;
}

static void arrange(struct lume_layer_shell *shell)
{
    struct lume_layer_rect area = full_area(shell);
    struct layer_surface *surface;
    for (int layer = 3; layer >= 0; --layer)
        wl_list_for_each(surface, &shell->surfaces,
                         link) if (surface->initialized &&
                                   surface->current.layer == (unsigned)layer &&
                                   surface->current.exclusive > 0)
        {
            surface->bounds = area;
            if (weston_surface_is_mapped(surface->surface))
                lume_layer_reserve(&surface->current, &area);
        }
    bool changed = memcmp(&area, &shell->work_area, sizeof(area)) != 0;
    shell->work_area = area;
    wl_list_for_each(surface, &shell->surfaces, link)
    {
        if (!surface->initialized)
            continue;
        struct lume_layer_rect bounds = surface->current.exclusive < 0   ? full_area(shell)
                                        : surface->current.exclusive > 0 ? surface->bounds
                                                                         : area;
        struct lume_layer_rect rectangle = lume_layer_place(&surface->current, bounds);
        if ((uint32_t)rectangle.width != surface->configured_width ||
            (uint32_t)rectangle.height != surface->configured_height)
        {
            struct layer_configure *configure = calloc(1, sizeof(*configure));
            if (!configure || surface->configure_count >= 64)
            {
                free(configure);
                wl_client_post_no_memory(wl_resource_get_client(surface->resource));
                continue;
            }
            configure->serial = wl_display_next_serial(shell->compositor->wl_display);
            wl_list_insert(surface->configures.prev, &configure->link);
            ++surface->configure_count;
            surface->configured_width = rectangle.width;
            surface->configured_height = rectangle.height;
            zwlr_layer_surface_v1_send_configure(surface->resource, configure->serial,
                                                 rectangle.width, rectangle.height);
        }
        struct weston_coord_global previous = weston_view_get_pos_offset_global(surface->view);
        struct weston_layer *layer = &shell->layers[surface->current.layer];
        bool moved = previous.c.x != rectangle.x || previous.c.y != rectangle.y ||
                     surface->view->layer_link.layer != layer;
        if (moved)
        {
            weston_view_set_position(surface->view,
                                     (struct weston_coord_global){{rectangle.x, rectangle.y}});
            if (weston_surface_is_mapped(surface->surface))
            {
                weston_view_move_to_layer(surface->view, &layer->view_list);
                weston_surface_damage(surface->surface);
            }
        }
        weston_view_update_transform(surface->view);
    }
    if (changed && shell->changed)
        shell->changed(shell->data);
}

static void layer_committed(struct weston_surface *base, struct weston_coord_surface origin)
{
    (void)origin;
    struct layer_surface *surface = base->committed_private;
    if (!lume_layer_valid(&surface->pending))
    {
        wl_resource_post_error(surface->resource, ZWLR_LAYER_SURFACE_V1_ERROR_INVALID_SIZE,
                               "Invalid layer surface dimensions or anchors");
        return;
    }
    if (weston_surface_has_content(base) && !surface->acknowledged)
    {
        wl_resource_post_error(surface->resource, ZWLR_LAYER_SURFACE_V1_ERROR_INVALID_SURFACE_STATE,
                               "Layer buffer requires an acknowledged configure");
        return;
    }
    surface->current = surface->pending;
    surface->initialized = true;
    bool mapped = weston_surface_is_mapped(base);
    if (weston_surface_has_content(base))
    {
        if (!mapped)
        {
            weston_surface_map(base);
            fprintf(stderr, "WAYLAND_LAYER mapped=%s layer=%u width=%d height=%d\n", surface->name,
                    surface->current.layer, base->width, base->height);
            if (surface->current.keyboard == ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE)
                weston_view_activate_input(surface->view, surface->shell->seat,
                                           WESTON_ACTIVATE_FLAG_NONE);
        }
    }
    else if (mapped)
    {
        weston_surface_unmap(base);
        surface->initialized = false;
        surface->acknowledged = false;
        surface->configured_width = surface->configured_height = 0;
        clear_configures(surface);
    }
    arrange(surface->shell);
}

static void destroy_request(struct wl_client *client, struct wl_resource *resource)
{
    (void)client;
    wl_resource_destroy(resource);
}

static void set_size(struct wl_client *client, struct wl_resource *resource, uint32_t width,
                     uint32_t height)
{
    (void)client;
    struct layer_surface *surface = wl_resource_get_user_data(resource);
    surface->pending.width = width;
    surface->pending.height = height;
}

static void set_anchor(struct wl_client *client, struct wl_resource *resource, uint32_t anchor)
{
    (void)client;
    struct layer_surface *surface = wl_resource_get_user_data(resource);
    if (anchor & ~15u)
        wl_resource_post_error(resource, ZWLR_LAYER_SURFACE_V1_ERROR_INVALID_ANCHOR,
                               "Invalid layer anchor");
    else
        surface->pending.anchor = anchor;
}

static void set_zone(struct wl_client *client, struct wl_resource *resource, int32_t zone)
{
    (void)client;
    struct layer_surface *surface = wl_resource_get_user_data(resource);
    surface->pending.exclusive = zone;
}

static void set_margin(struct wl_client *client, struct wl_resource *resource, int32_t top,
                       int32_t right, int32_t bottom, int32_t left)
{
    (void)client;
    struct layer_surface *surface = wl_resource_get_user_data(resource);
    surface->pending.top = top;
    surface->pending.right = right;
    surface->pending.bottom = bottom;
    surface->pending.left = left;
}

static void set_keyboard(struct wl_client *client, struct wl_resource *resource, uint32_t mode)
{
    (void)client;
    struct layer_surface *surface = wl_resource_get_user_data(resource);
    if (mode > 2 || (mode == 2 && wl_resource_get_version(resource) < 4))
        wl_resource_post_error(resource, ZWLR_LAYER_SURFACE_V1_ERROR_INVALID_KEYBOARD_INTERACTIVITY,
                               "Invalid layer keyboard mode");
    else
        surface->pending.keyboard = mode;
}

static void set_layer(struct wl_client *client, struct wl_resource *resource, uint32_t layer)
{
    (void)client;
    struct layer_surface *surface = wl_resource_get_user_data(resource);
    if (layer > 3)
        wl_resource_post_error(resource, ZWLR_LAYER_SHELL_V1_ERROR_INVALID_LAYER,
                               "Invalid surface layer");
    else
        surface->pending.layer = layer;
}

static void get_popup(struct wl_client *client, struct wl_resource *resource,
                      struct wl_resource *popup)
{
    (void)client;
    struct layer_surface *surface = wl_resource_get_user_data(resource);
    if (!lume_desktop_popup_set_layer_parent(popup, surface->view))
        wl_resource_post_error(resource, ZWLR_LAYER_SURFACE_V1_ERROR_INVALID_SURFACE_STATE,
                               "Invalid layer popup parent");
}

static void ack_configure(struct wl_client *client, struct wl_resource *resource, uint32_t serial)
{
    (void)client;
    struct layer_surface *surface = wl_resource_get_user_data(resource);
    struct layer_configure *configure, *found = NULL;
    wl_list_for_each(configure, &surface->configures, link) if (configure->serial == serial) found =
        configure;
    if (!found)
    {
        wl_resource_post_error(resource, ZWLR_LAYER_SURFACE_V1_ERROR_INVALID_SURFACE_STATE,
                               "Unknown layer configure serial");
        return;
    }
    while (!wl_list_empty(&surface->configures))
    {
        configure = wl_container_of(surface->configures.next, configure, link);
        bool last = configure == found;
        wl_list_remove(&configure->link);
        free(configure);
        --surface->configure_count;
        if (last)
            break;
    }
    surface->acknowledged = true;
}

static const struct zwlr_layer_surface_v1_interface surface_interface = {
    .set_size = set_size,
    .set_anchor = set_anchor,
    .set_exclusive_zone = set_zone,
    .set_margin = set_margin,
    .set_keyboard_interactivity = set_keyboard,
    .get_popup = get_popup,
    .ack_configure = ack_configure,
    .destroy = destroy_request,
    .set_layer = set_layer};

static void surface_destroyed(struct wl_listener *listener, void *data)
{
    (void)data;
    struct layer_surface *surface = wl_container_of(listener, surface, destroyed);
    surface->surface = NULL;
    wl_resource_destroy(surface->resource);
}

static void release_surface(struct wl_resource *resource)
{
    struct layer_surface *surface = wl_resource_get_user_data(resource);
    struct lume_layer_shell *shell = surface->shell;
    wl_list_remove(&surface->link);
    wl_list_remove(&surface->destroyed.link);
    if (surface->surface)
    {
        if (weston_surface_is_mapped(surface->surface))
            weston_surface_unmap(surface->surface);
        surface->surface->committed = NULL;
        surface->surface->committed_private = NULL;
    }
    weston_view_destroy(surface->view);
    clear_configures(surface);
    free(surface);
    arrange(shell);
}

static void get_layer_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id,
                              struct wl_resource *surface_resource, struct wl_resource *output,
                              uint32_t layer, const char *name)
{
    struct lume_layer_shell *shell = wl_resource_get_user_data(resource);
    struct weston_surface *base = wl_resource_get_user_data(surface_resource);
    struct weston_head *head = output ? weston_head_from_resource(output) : NULL;
    if (layer > 3 || (output && (!head || head->output != shell->output)))
    {
        wl_resource_post_error(resource, ZWLR_LAYER_SHELL_V1_ERROR_INVALID_LAYER,
                               "Unsupported layer or output");
        return;
    }
    if (base->committed || weston_surface_has_content(base))
    {
        wl_resource_post_error(resource, ZWLR_LAYER_SHELL_V1_ERROR_ALREADY_CONSTRUCTED,
                               "Layer surface is already constructed");
        return;
    }
    if (weston_surface_set_role(base, "zwlr_layer_surface_v1", resource,
                                ZWLR_LAYER_SHELL_V1_ERROR_ROLE) < 0)
        return;
    struct layer_surface *surface = calloc(1, sizeof(*surface));
    if (!surface)
    {
        wl_client_post_no_memory(client);
        return;
    }
    surface->resource = wl_resource_create(client, &zwlr_layer_surface_v1_interface,
                                           wl_resource_get_version(resource), id);
    surface->view = weston_view_create(base);
    if (!surface->resource || !surface->view)
    {
        if (surface->resource)
            wl_resource_destroy(surface->resource);
        if (surface->view)
            weston_view_destroy(surface->view);
        free(surface);
        wl_client_post_no_memory(client);
        return;
    }
    surface->shell = shell;
    surface->surface = base;
    surface->pending.layer = layer;
    snprintf(surface->name, sizeof(surface->name), "%s", name);
    wl_list_init(&surface->configures);
    wl_list_insert(shell->surfaces.prev, &surface->link);
    surface->destroyed.notify = surface_destroyed;
    wl_signal_add(&base->destroy_signal, &surface->destroyed);
    wl_resource_set_implementation(surface->resource, &surface_interface, surface, release_surface);
    base->committed = layer_committed;
    base->committed_private = surface;
}

static const struct zwlr_layer_shell_v1_interface shell_interface = {
    .get_layer_surface = get_layer_surface, .destroy = destroy_request};

static void bind_shell(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    struct wl_resource *resource =
        wl_resource_create(client, &zwlr_layer_shell_v1_interface, version < 4 ? version : 4, id);
    if (resource)
        wl_resource_set_implementation(resource, &shell_interface, data, NULL);
    else
        wl_client_post_no_memory(client);
}

static void focus_layer(struct weston_pointer *pointer, const struct timespec *time,
                        uint32_t button, void *data)
{
    (void)time;
    (void)button;
    (void)data;
    struct weston_view *view = pointer->focus;
    if (view && view->surface->committed == layer_committed)
    {
        struct layer_surface *surface = view->surface->committed_private;
        if (surface->current.keyboard)
            weston_view_activate_input(view, pointer->seat, WESTON_ACTIVATE_FLAG_CLICKED);
    }
}

struct lume_layer_shell *lume_layer_shell_create(struct weston_compositor *compositor,
                                                 struct weston_output *output,
                                                 struct weston_seat *seat, void (*changed)(void *),
                                                 void *data)
{
    struct lume_layer_shell *shell = calloc(1, sizeof(*shell));
    if (!shell)
        return NULL;
    shell->compositor = compositor;
    shell->output = output;
    shell->seat = seat;
    shell->changed = changed;
    shell->data = data;
    shell->work_area = full_area(shell);
    wl_list_init(&shell->surfaces);

    enum weston_layer_position positions[] = {
        WESTON_LAYER_POSITION_BACKGROUND + 1, WESTON_LAYER_POSITION_BOTTOM_UI,
        WESTON_LAYER_POSITION_FULLSCREEN - 1, WESTON_LAYER_POSITION_LOCK - 1};

    for (unsigned i = 0; i < 4; ++i)
    {
        weston_layer_init(shell->layers + i, compositor);
        weston_layer_set_position(shell->layers + i, positions[i]);
    }
    shell->global = wl_global_create(compositor->wl_display, &zwlr_layer_shell_v1_interface, 4,
                                     shell, bind_shell);
    if (!shell->global)
    {
        lume_layer_shell_destroy(shell);
        return NULL;
    }
    weston_compositor_add_button_binding(compositor, BTN_LEFT, 0, focus_layer, shell);
    return shell;
}

void lume_layer_shell_destroy(struct lume_layer_shell *shell)
{
    if (!shell)
        return;
    if (shell->global)
        wl_global_destroy(shell->global);
    while (!wl_list_empty(&shell->surfaces))
    {
        struct layer_surface *surface = wl_container_of(shell->surfaces.next, surface, link);
        wl_resource_destroy(surface->resource);
    }
    for (unsigned i = 0; i < 4; ++i)
        weston_layer_fini(shell->layers + i);
    free(shell);
}

struct lume_layer_rect lume_layer_shell_work_area(struct lume_layer_shell *shell)
{
    return shell->work_area;
}
