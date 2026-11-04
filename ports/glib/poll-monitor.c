/* SPDX-License-Identifier: BSD-3-Clause */
#include "config.h"
#include "gio/glocalfilemonitor.h"
#include "gio/gfile.h"
#include "gio/gfileenumerator.h"
#include "gio/gfileinfo.h"
#include "gio/gioerror.h"
#include "glib/glib-private.h"

typedef struct
{
    GLocalFileMonitor parent;
    GSource *timer;
} LumePollMonitor;

typedef GLocalFileMonitorClass LumePollMonitorClass;

typedef struct
{
    GFile *file;
    gchar *basename;
    gboolean directory;
    GFileMonitorSource *source;
    GHashTable *entries;
} PollWatch;

GType lume_poll_monitor_get_type(void);
G_DEFINE_TYPE(LumePollMonitor, lume_poll_monitor, G_TYPE_LOCAL_FILE_MONITOR)

#define ATTRIBUTES                                                                                 \
    "standard::name,standard::type,standard::size,time::modified,"                                 \
    "time::modified-usec,unix::mode,id::file"

static GHashTable *snapshot(PollWatch *watch)
{
    GError *error = NULL;
    GHashTable *entries = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);
    GFileInfo *info = g_file_query_info(watch->file, ATTRIBUTES,
                                        G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, &error);
    if (!info)
    {
        gboolean absent = g_error_matches(error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND);
        g_clear_error(&error);
        if (absent)
            return entries;
        g_hash_table_unref(entries);
        return NULL;
    }
    g_hash_table_insert(entries, g_strdup(""), info);
    if (!watch->directory)
        return entries;
    GFileEnumerator *enumerator = g_file_enumerate_children(
        watch->file, ATTRIBUTES, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, &error);
    if (enumerator)
    {
        while ((info = g_file_enumerator_next_file(enumerator, NULL, &error)))
            g_hash_table_insert(entries, g_strdup(g_file_info_get_name(info)), info);
        g_file_enumerator_close(enumerator, NULL, NULL);
        g_object_unref(enumerator);
    }
    if (error)
    {
        g_clear_error(&error);
        g_hash_table_unref(entries);
        return NULL;
    }
    return entries;
}

static gboolean same_entry(GFileInfo *first, GFileInfo *second)
{
    return g_file_info_get_file_type(first) == g_file_info_get_file_type(second) &&
           g_file_info_get_size(first) == g_file_info_get_size(second) &&
           g_file_info_get_attribute_uint64(first, G_FILE_ATTRIBUTE_TIME_MODIFIED) ==
               g_file_info_get_attribute_uint64(second, G_FILE_ATTRIBUTE_TIME_MODIFIED) &&
           g_file_info_get_attribute_uint32(first, G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC) ==
               g_file_info_get_attribute_uint32(second, G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC) &&
           g_file_info_get_attribute_uint32(first, G_FILE_ATTRIBUTE_UNIX_MODE) ==
               g_file_info_get_attribute_uint32(second, G_FILE_ATTRIBUTE_UNIX_MODE) &&
           g_strcmp0(g_file_info_get_attribute_string(first, G_FILE_ATTRIBUTE_ID_FILE),
                     g_file_info_get_attribute_string(second, G_FILE_ATTRIBUTE_ID_FILE)) == 0;
}

static void emit(PollWatch *watch, const char *name, GFileMonitorEvent event, gint64 now)
{
    g_file_monitor_source_handle_event(watch->source, event, *name ? name : watch->basename, NULL,
                                       NULL, now);
}

static gboolean poll_watch(gpointer opaque)
{
    PollWatch *watch = opaque;
    GHashTable *next = snapshot(watch);
    if (!next)
        return G_SOURCE_CONTINUE;
    GHashTableIter iterator;
    gpointer name, info;
    gint64 now = g_get_monotonic_time();
    g_hash_table_iter_init(&iterator, next);
    while (g_hash_table_iter_next(&iterator, &name, &info))
    {
        GFileInfo *previous = g_hash_table_lookup(watch->entries, name);
        if (!previous)
            emit(watch, name, G_FILE_MONITOR_EVENT_CREATED, now);
        else if (!same_entry(previous, info))
            emit(watch, name, G_FILE_MONITOR_EVENT_CHANGED, now);
    }
    g_hash_table_iter_init(&iterator, watch->entries);
    while (g_hash_table_iter_next(&iterator, &name, NULL))
        if (!g_hash_table_contains(next, name))
            emit(watch, name, G_FILE_MONITOR_EVENT_DELETED, now);
    g_hash_table_unref(watch->entries);
    watch->entries = next;
    return G_SOURCE_CONTINUE;
}

static void free_watch(gpointer opaque)
{
    PollWatch *watch = opaque;
    g_hash_table_unref(watch->entries);
    g_object_unref(watch->file);
    g_free(watch->basename);
    g_source_unref((GSource *)watch->source);
    g_free(watch);
}

static void start_watch(GLocalFileMonitor *monitor, const gchar *directory, const gchar *basename,
                        const gchar *filename, GFileMonitorSource *source)
{
    LumePollMonitor *poll = (LumePollMonitor *)monitor;
    PollWatch *watch = g_new0(PollWatch, 1);
    gchar *path = filename   ? g_strdup(filename)
                  : basename ? g_build_filename(directory, basename, NULL)
                             : g_strdup(directory);
    watch->directory = !filename && !basename;
    watch->basename = g_strdup(basename);
    watch->file = g_file_new_for_path(path);
    g_free(path);
    watch->source = (GFileMonitorSource *)g_source_ref((GSource *)source);
    watch->entries = snapshot(watch);
    if (!watch->entries)
        watch->entries = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);
    poll->timer = g_timeout_source_new(1000);
    g_source_set_static_name(poll->timer, "Lume file polling");
    g_source_set_callback(poll->timer, poll_watch, watch, free_watch);
    g_source_attach(poll->timer, GLIB_PRIVATE_CALL(g_get_worker_context)());
}

static gboolean cancel_watch(GFileMonitor *monitor)
{
    LumePollMonitor *poll = (LumePollMonitor *)monitor;
    if (poll->timer)
    {
        g_source_destroy(poll->timer);
        g_source_unref(poll->timer);
        poll->timer = NULL;
    }
    return TRUE;
}

static void lume_poll_monitor_class_init(LumePollMonitorClass *type)
{
    type->start = start_watch;
    G_FILE_MONITOR_CLASS(type)->cancel = cancel_watch;
}

static void lume_poll_monitor_init(LumePollMonitor *monitor)
{
    (void)monitor;
}
