/* SPDX-License-Identifier: BSD-3-Clause */
#include "toplevel.h"
#include "wlr-foreign-toplevel-management-unstable-v1-server-protocol.h"
#include <stdlib.h>
#include <string.h>

struct lume_toplevel_manager
{
    struct wl_global *global;
    struct weston_head *head;
    struct weston_seat *seat;
    struct wl_list windows, bindings;
    void (*action)(struct weston_desktop_surface *, enum lume_toplevel_action, bool, void *);
    void *data;
};

struct toplevel_binding
{
    struct lume_toplevel_manager *manager;
    struct wl_resource *resource;
    struct wl_list link, handles;
};

struct lume_toplevel
{
    struct lume_toplevel_manager *manager;
    struct weston_desktop_surface *surface;
    struct wl_list link, handles;
    struct wl_listener metadata;
    char *title, *app_id;
    unsigned state;
    bool minimized;
};

struct toplevel_handle
{
    struct wl_resource *resource;
    struct lume_toplevel *window;
    struct toplevel_binding *binding;
    struct wl_list window_link, binding_link;
};

static void handle_released(struct wl_resource *resource)
{
    struct toplevel_handle *handle = wl_resource_get_user_data(resource);
    wl_list_remove(&handle->window_link);
    wl_list_remove(&handle->binding_link);
    free(handle);
}

static void destroy_request(struct wl_client *client, struct wl_resource *resource)
{
    (void)client;
    wl_resource_destroy(resource);
}

static void action(struct wl_resource *resource, enum lume_toplevel_action operation, bool value)
{
    struct toplevel_handle *handle = wl_resource_get_user_data(resource);
    if (handle->window)
    {
        struct lume_toplevel *window = handle->window;
        window->manager->action(window->surface, operation, value, window->manager->data);
    }
}

static void maximize(struct wl_client *client, struct wl_resource *resource)
{
    (void)client;
    action(resource, LUME_TOPLEVEL_MAXIMIZE, true);
}

static void unmaximize(struct wl_client *client, struct wl_resource *resource)
{
    (void)client;
    action(resource, LUME_TOPLEVEL_MAXIMIZE, false);
}

static void minimize(struct wl_client *client, struct wl_resource *resource)
{
    (void)client;
    action(resource, LUME_TOPLEVEL_MINIMIZE, true);
}

static void unminimize(struct wl_client *client, struct wl_resource *resource)
{
    (void)client;
    action(resource, LUME_TOPLEVEL_MINIMIZE, false);
}

static void activate(struct wl_client *client, struct wl_resource *resource,
                     struct wl_resource *seat)
{
    (void)client;
    struct toplevel_handle *handle = wl_resource_get_user_data(resource);
    if (handle->window && wl_resource_get_user_data(seat) == handle->window->manager->seat)
        action(resource, LUME_TOPLEVEL_ACTIVATE, true);
}

static void close_window(struct wl_client *client, struct wl_resource *resource)
{
    (void)client;
    action(resource, LUME_TOPLEVEL_CLOSE, true);
}

static void set_rectangle(struct wl_client *client, struct wl_resource *resource,
                          struct wl_resource *surface, int32_t x, int32_t y, int32_t width,
                          int32_t height)
{
    (void)client;
    (void)surface;
    (void)x;
    (void)y;
    struct toplevel_handle *handle = wl_resource_get_user_data(resource);
    if (handle->window && (width < 0 || height < 0))
        wl_resource_post_error(resource, ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_ERROR_INVALID_RECTANGLE,
                               "Invalid window rectangle");
}

static void fullscreen(struct wl_client *client, struct wl_resource *resource,
                       struct wl_resource *output)
{
    (void)client;
    (void)output;
    action(resource, LUME_TOPLEVEL_FULLSCREEN, true);
}

static void unfullscreen(struct wl_client *client, struct wl_resource *resource)
{
    (void)client;
    action(resource, LUME_TOPLEVEL_FULLSCREEN, false);
}

static const struct zwlr_foreign_toplevel_handle_v1_interface handle_interface = {
    .set_maximized = maximize,
    .unset_maximized = unmaximize,
    .set_minimized = minimize,
    .unset_minimized = unminimize,
    .activate = activate,
    .close = close_window,
    .set_rectangle = set_rectangle,
    .destroy = destroy_request,
    .set_fullscreen = fullscreen,
    .unset_fullscreen = unfullscreen};

static void send_state(struct toplevel_handle *handle)
{
    uint32_t values[4];
    size_t count = 0;
    for (unsigned i = 0; i < 4; ++i)
        if (handle->window->state & (1u << i))
            values[count++] = i;
    struct wl_array array = {count * sizeof(*values), sizeof(values), values};
    zwlr_foreign_toplevel_handle_v1_send_state(handle->resource, &array);
}

static struct toplevel_handle *add_handle(struct lume_toplevel *window,
                                          struct toplevel_binding *binding)
{
    struct toplevel_handle *handle = calloc(1, sizeof(*handle));
    struct wl_client *client = wl_resource_get_client(binding->resource);
    if (!handle)
    {
        wl_client_post_no_memory(client);
        return NULL;
    }
    handle->resource = wl_resource_create(client, &zwlr_foreign_toplevel_handle_v1_interface,
                                          wl_resource_get_version(binding->resource), 0);
    if (!handle->resource)
    {
        free(handle);
        wl_client_post_no_memory(client);
        return NULL;
    }
    handle->window = window;
    handle->binding = binding;
    wl_list_insert(window->handles.prev, &handle->window_link);
    wl_list_insert(binding->handles.prev, &handle->binding_link);
    wl_resource_set_implementation(handle->resource, &handle_interface, handle, handle_released);
    zwlr_foreign_toplevel_manager_v1_send_toplevel(binding->resource, handle->resource);
    zwlr_foreign_toplevel_handle_v1_send_title(handle->resource, window->title);
    zwlr_foreign_toplevel_handle_v1_send_app_id(handle->resource, window->app_id);
    struct wl_resource *output;
    wl_resource_for_each(
        output, &window->manager->head->resource_list) if (wl_resource_get_client(output) == client)
        zwlr_foreign_toplevel_handle_v1_send_output_enter(handle->resource, output);
    send_state(handle);
    zwlr_foreign_toplevel_handle_v1_send_done(handle->resource);
    return handle;
}

void lume_toplevel_update(struct lume_toplevel *window, bool minimized)
{
    if (!window)
        return;
    struct weston_desktop_surface *surface = window->surface;
    unsigned state = (weston_desktop_surface_get_pending_maximized(surface) ? 1u : 0u) |
                     (minimized ? 2u : 0u) |
                     (weston_desktop_surface_get_pending_activated(surface) ? 4u : 0u) |
                     (weston_desktop_surface_get_pending_fullscreen(surface) ? 8u : 0u);
    const char *title = weston_desktop_surface_get_title(surface);
    const char *app_id = weston_desktop_surface_get_app_id(surface);
    title = title ? title : "";
    app_id = app_id ? app_id : "";
    bool title_changed = !window->title || strcmp(title, window->title);
    bool app_changed = !window->app_id || strcmp(app_id, window->app_id);
    bool state_changed = state != window->state;
    char *new_title = title_changed ? strdup(title) : NULL;
    char *new_app_id = app_changed ? strdup(app_id) : NULL;
    if ((title_changed && !new_title) || (app_changed && !new_app_id))
    {
        free(new_title);
        free(new_app_id);
        return;
    }
    if (title_changed)
    {
        free(window->title);
        window->title = new_title;
    }
    if (app_changed)
    {
        free(window->app_id);
        window->app_id = new_app_id;
    }
    window->state = state;
    window->minimized = minimized;
    if (!title_changed && !app_changed && !state_changed)
        return;
    struct toplevel_handle *handle;
    wl_list_for_each(handle, &window->handles, window_link)
    {
        if (title_changed)
            zwlr_foreign_toplevel_handle_v1_send_title(handle->resource, title);
        if (app_changed)
            zwlr_foreign_toplevel_handle_v1_send_app_id(handle->resource, app_id);
        if (state_changed)
            send_state(handle);
        zwlr_foreign_toplevel_handle_v1_send_done(handle->resource);
    }
}

static void metadata_changed(struct wl_listener *listener, void *data)
{
    (void)data;
    struct lume_toplevel *window = wl_container_of(listener, window, metadata);
    lume_toplevel_update(window, window->minimized);
}

struct lume_toplevel *lume_toplevel_add(struct lume_toplevel_manager *manager,
                                        struct weston_desktop_surface *surface)
{
    struct lume_toplevel *window = calloc(1, sizeof(*window));
    if (!window)
        return NULL;
    window->manager = manager;
    window->surface = surface;
    wl_list_init(&window->handles);
    lume_toplevel_update(window, false);
    if (!window->title || !window->app_id)
    {
        free(window->title);
        free(window->app_id);
        free(window);
        return NULL;
    }
    wl_list_insert(manager->windows.prev, &window->link);
    window->metadata.notify = metadata_changed;
    weston_desktop_surface_add_metadata_listener(surface, &window->metadata);
    struct toplevel_binding *binding;
    wl_list_for_each(binding, &manager->bindings, link) add_handle(window, binding);
    return window;
}

void lume_toplevel_remove(struct lume_toplevel *window)
{
    if (!window)
        return;
    struct toplevel_handle *handle, *next;
    wl_list_for_each_safe(handle, next, &window->handles, window_link)
    {
        zwlr_foreign_toplevel_handle_v1_send_closed(handle->resource);
        handle->window = NULL;
        wl_list_remove(&handle->window_link);
        wl_list_init(&handle->window_link);
    }
    wl_list_remove(&window->metadata.link);
    wl_list_remove(&window->link);
    free(window->title);
    free(window->app_id);
    free(window);
}

static void close_binding_handles(struct toplevel_binding *binding)
{
    struct toplevel_handle *handle, *next;
    wl_list_for_each_safe(handle, next, &binding->handles, binding_link)
    {
        if (handle->window)
        {
            zwlr_foreign_toplevel_handle_v1_send_closed(handle->resource);
            wl_list_remove(&handle->window_link);
            wl_list_init(&handle->window_link);
        }
        handle->window = NULL;
        handle->binding = NULL;
        wl_list_remove(&handle->binding_link);
        wl_list_init(&handle->binding_link);
    }
}

static void binding_released(struct wl_resource *resource)
{
    struct toplevel_binding *binding = wl_resource_get_user_data(resource);
    close_binding_handles(binding);
    wl_list_remove(&binding->link);
    free(binding);
}

static void stop_manager(struct wl_client *client, struct wl_resource *resource)
{
    (void)client;
    close_binding_handles(wl_resource_get_user_data(resource));
    zwlr_foreign_toplevel_manager_v1_send_finished(resource);
    wl_resource_destroy(resource);
}

static const struct zwlr_foreign_toplevel_manager_v1_interface manager_interface = {
    .stop = stop_manager};

static void bind_manager(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    struct lume_toplevel_manager *manager = data;
    struct toplevel_binding *binding = calloc(1, sizeof(*binding));
    if (!binding)
    {
        wl_client_post_no_memory(client);
        return;
    }
    binding->resource = wl_resource_create(client, &zwlr_foreign_toplevel_manager_v1_interface,
                                           version < 2 ? version : 2, id);
    if (!binding->resource)
    {
        free(binding);
        wl_client_post_no_memory(client);
        return;
    }
    binding->manager = manager;
    wl_list_init(&binding->handles);
    wl_list_insert(manager->bindings.prev, &binding->link);
    wl_resource_set_implementation(binding->resource, &manager_interface, binding,
                                   binding_released);
    struct lume_toplevel *window;
    wl_list_for_each(window, &manager->windows, link) add_handle(window, binding);
}

struct lume_toplevel_manager *
lume_toplevel_manager_create(struct weston_compositor *compositor, struct weston_head *head,
                             struct weston_seat *seat,
                             void (*action_callback)(struct weston_desktop_surface *,
                                                     enum lume_toplevel_action, bool, void *),
                             void *data)
{
    struct lume_toplevel_manager *manager = calloc(1, sizeof(*manager));
    if (!manager)
        return NULL;
    manager->head = head;
    manager->seat = seat;
    manager->action = action_callback;
    manager->data = data;
    wl_list_init(&manager->windows);
    wl_list_init(&manager->bindings);
    manager->global =
        wl_global_create(compositor->wl_display, &zwlr_foreign_toplevel_manager_v1_interface, 2,
                         manager, bind_manager);
    if (!manager->global)
    {
        free(manager);
        return NULL;
    }
    return manager;
}

void lume_toplevel_manager_destroy(struct lume_toplevel_manager *manager)
{
    if (!manager)
        return;
    wl_global_destroy(manager->global);
    while (!wl_list_empty(&manager->windows))
    {
        struct lume_toplevel *window = wl_container_of(manager->windows.next, window, link);
        lume_toplevel_remove(window);
    }
    while (!wl_list_empty(&manager->bindings))
    {
        struct toplevel_binding *binding = wl_container_of(manager->bindings.next, binding, link);
        wl_resource_destroy(binding->resource);
    }
    free(manager);
}
