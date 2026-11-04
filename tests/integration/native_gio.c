/* SPDX-License-Identifier: BSD-3-Clause */
#include <gio/gio.h>
#include <gio/gunixsocketaddress.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static void work(GTask *task, gpointer source, gpointer data, GCancellable *cancellable)
{
    (void)source;
    (void)cancellable;
    g_assert(data == task);
    g_task_return_int(task, 42);
}

struct async_files
{
    GMainLoop *loop;
    unsigned pending;
};

static void query_ready(GObject *source, GAsyncResult *result, gpointer opaque)
{
    struct async_files *state = opaque;
    GError *error = NULL;
    GFileInfo *info = g_file_query_info_finish(G_FILE(source), result, &error);
    g_assert_no_error(error);
    g_assert(info && g_file_info_get_file_type(info) == G_FILE_TYPE_REGULAR);
    g_assert_cmpstr(g_file_info_get_name(info), ==, "ação.txt");
    g_object_unref(info);
    if (!--state->pending)
        g_main_loop_quit(state->loop);
}

static void enumerate_ready(GObject *source, GAsyncResult *result, gpointer opaque)
{
    struct async_files *state = opaque;
    GError *error = NULL;
    GFileEnumerator *entries = g_file_enumerate_children_finish(G_FILE(source), result, &error);
    g_assert_no_error(error);
    g_assert(entries);
    unsigned count = 0;
    GFileInfo *info;
    while ((info = g_file_enumerator_next_file(entries, NULL, &error)))
    {
        g_assert(g_file_info_get_name(info));
        g_object_unref(info);
        ++count;
    }
    g_assert_no_error(error);
    g_assert(count == 2 && g_file_enumerator_close(entries, NULL, &error));
    g_assert_no_error(error);
    g_object_unref(entries);
    if (!--state->pending)
        g_main_loop_quit(state->loop);
}

static void async_files(GFile *directory, GFile *file)
{
    struct async_files state = {.loop = g_main_loop_new(NULL, FALSE)};
    for (unsigned batch = 0; batch < 16; ++batch)
    {
        state.pending = 4;
        for (unsigned index = 0; index < 2; ++index)
        {
            g_file_query_info_async(file, "standard::name,standard::type,standard::size",
                                    G_FILE_QUERY_INFO_NONE, 0, NULL, query_ready, &state);
            g_file_enumerate_children_async(directory, "standard::name,standard::type",
                                            G_FILE_QUERY_INFO_NONE, 0, NULL, enumerate_ready,
                                            &state);
        }
        g_main_loop_run(state.loop);
        g_assert(!state.pending);
    }
    g_main_loop_unref(state.loop);
    puts("GIO_ASYNC_FILES_PASS operations=64");
}

struct monitor_events
{
    GFile *file;
    unsigned created;
    unsigned changed;
    unsigned deleted;
};

static void monitor_event(GFileMonitor *monitor, GFile *file, GFile *other, GFileMonitorEvent event,
                          gpointer opaque)
{
    (void)monitor;
    (void)other;
    struct monitor_events *events = opaque;
    if (!g_file_equal(file, events->file))
        return;
    events->created += event == G_FILE_MONITOR_EVENT_CREATED;
    events->changed += event == G_FILE_MONITOR_EVENT_CHANGED;
    events->deleted += event == G_FILE_MONITOR_EVENT_DELETED;
}

static void wait_monitor(unsigned *first, unsigned *second)
{
    gint64 deadline = g_get_monotonic_time() + 15 * G_TIME_SPAN_SECOND;
    while ((!*first || !*second) && g_get_monotonic_time() < deadline)
    {
        while (g_main_context_iteration(NULL, FALSE))
        {
        }
        g_usleep(10000);
    }
    g_assert(*first && *second);
}

static void file_monitors(GFile *directory)
{
    GError *error = NULL;
    GFile *file = g_file_get_child(directory, "avaliação.txt");
    GFileMonitor *parent = g_file_monitor_directory(directory, 0, NULL, &error);
    g_assert_no_error(error);
    g_assert(parent);
    GFileMonitor *child = g_file_monitor_file(file, 0, NULL, &error);
    g_assert_no_error(error);
    g_assert(child);
    struct monitor_events parent_events = {.file = file};
    struct monitor_events child_events = {.file = file};
    g_signal_connect(parent, "changed", G_CALLBACK(monitor_event), &parent_events);
    g_signal_connect(child, "changed", G_CALLBACK(monitor_event), &child_events);
    gchar *path = g_file_get_path(file);
    int descriptor = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
    g_assert(descriptor >= 0 && write(descriptor, "a", 1) == 1 && !close(descriptor));
    wait_monitor(&parent_events.created, &child_events.created);
    descriptor = open(path, O_WRONLY | O_APPEND);
    g_assert(descriptor >= 0 && write(descriptor, "café", 5) == 5 && !close(descriptor));
    wait_monitor(&parent_events.changed, &child_events.changed);
    g_assert(!unlink(path));
    wait_monitor(&parent_events.deleted, &child_events.deleted);
    g_assert(g_file_monitor_cancel(parent) && g_file_monitor_cancel(child));
    g_assert(g_file_monitor_is_cancelled(parent) && g_file_monitor_is_cancelled(child));
    unsigned before = parent_events.created + child_events.created;
    descriptor = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
    g_assert(descriptor >= 0 && !close(descriptor));
    gint64 deadline = g_get_monotonic_time() + 2 * G_TIME_SPAN_SECOND;
    while (g_get_monotonic_time() < deadline)
    {
        while (g_main_context_iteration(NULL, FALSE))
        {
        }
        g_usleep(10000);
    }
    g_assert(parent_events.created + child_events.created == before);
    g_object_unref(parent);
    g_object_unref(child);
    for (unsigned i = 0; i < 32; ++i)
    {
        child = g_file_monitor_file(file, 0, NULL, &error);
        g_assert_no_error(error);
        g_assert(child && g_file_monitor_cancel(child));
        g_object_unref(child);
    }
    g_assert(!unlink(path));
    g_free(path);
    g_object_unref(file);
    puts("GIO_FILE_DIRECTORY_MONITOR_PASS create=2 change=2 delete=2 cancel=34");
}

int main(void)
{
    assert(setlocale(LC_ALL, "pt_PT.UTF-8"));
    GError *error = NULL;
    gchar *path = g_strdup_printf("/data/gio-check-%ld", (long)getpid());
    GFile *directory = g_file_new_for_path(path);
    g_free(path);
    g_assert(g_file_make_directory(directory, NULL, &error));
    g_assert_no_error(error);
    g_assert(g_file_query_exists(directory, NULL));
    GFile *accent_directory = g_file_get_child(directory, "informação");
    g_assert(g_file_make_directory(accent_directory, NULL, &error));
    g_assert_no_error(error);
    g_assert(g_file_query_exists(accent_directory, NULL));
    g_assert(g_file_query_file_type(accent_directory, 0, NULL) == G_FILE_TYPE_DIRECTORY);
    g_assert(g_file_delete(accent_directory, NULL, &error));
    g_assert_no_error(error);
    g_assert(!g_file_query_exists(accent_directory, NULL));
    g_object_unref(accent_directory);
    puts("GIO_EXISTING_MISSING_UTF8_DIRECTORY_PASS");
    GFileInfo *filesystem = g_file_query_filesystem_info(directory, "filesystem::*", NULL, &error);
    g_assert_no_error(error);
    g_assert(filesystem);
    g_assert(!g_file_info_has_attribute(filesystem, G_FILE_ATTRIBUTE_FILESYSTEM_TYPE));
    g_assert(!g_file_info_has_attribute(filesystem, G_FILE_ATTRIBUTE_FILESYSTEM_SIZE));
    g_assert(!g_file_info_has_attribute(filesystem, G_FILE_ATTRIBUTE_FILESYSTEM_FREE));
    g_object_unref(filesystem);
    puts("GIO_FILESYSTEM_UNAVAILABLE_PASS");
    GFile *original = g_file_get_child(directory, "ação.txt");
    GFile *copy = g_file_get_child(directory, "cópia.txt");
    GFile *moved = g_file_get_child(directory, "café.txt");
    const gchar contents[] = "ação e café\n";
    g_assert(g_file_replace_contents(original, contents, sizeof(contents) - 1, NULL, FALSE,
                                     G_FILE_CREATE_NONE, NULL, NULL, &error));
    g_assert_no_error(error);
    GFileInfo *info = g_file_query_info(
        original, "standard::name,standard::display-name,standard::type,standard::size",
        G_FILE_QUERY_INFO_NONE, NULL, &error);
    g_assert_no_error(error);
    g_assert(info && g_file_info_get_file_type(info) == G_FILE_TYPE_REGULAR);
    g_assert_cmpstr(g_file_info_get_name(info), ==, "ação.txt");
    g_assert_cmpstr(g_file_info_get_display_name(info), ==, "ação.txt");
    g_assert(g_file_info_get_size(info) == sizeof(contents) - 1);
    g_object_unref(info);
    g_assert(g_file_copy(original, copy, G_FILE_COPY_NONE, NULL, NULL, NULL, &error));
    g_assert_no_error(error);
    g_assert(g_file_move(copy, moved, G_FILE_COPY_NONE, NULL, NULL, NULL, &error));
    g_assert_no_error(error);
    gchar *loaded = NULL;
    gsize bytes;
    g_assert(g_file_load_contents(moved, NULL, &loaded, &bytes, NULL, &error));
    g_assert_no_error(error);
    g_assert(bytes == sizeof(contents) - 1 && !memcmp(loaded, contents, bytes));
    g_free(loaded);
    GFileEnumerator *entries = g_file_enumerate_children(directory, "standard::name",
                                                         G_FILE_QUERY_INFO_NONE, NULL, &error);
    g_assert_no_error(error);
    guint count = 0;
    while ((info = g_file_enumerator_next_file(entries, NULL, &error)))
    {
        ++count;
        g_object_unref(info);
    }
    g_assert_no_error(error);
    g_assert(count == 2 && g_file_enumerator_close(entries, NULL, &error));
    g_assert_no_error(error);
    g_object_unref(entries);
    gchar *directory_path = g_file_get_path(directory);
    int descriptor = open(directory_path, O_RDONLY | O_DIRECTORY);
    g_free(directory_path);
    g_assert(descriptor >= 0);
    DIR *stream = fdopendir(descriptor);
    g_assert(stream && dirfd(stream) == descriptor);
    count = 0;
    while (readdir(stream))
        ++count;
    g_assert(count == 2 && !closedir(stream));
    g_assert(fcntl(descriptor, F_GETFD) < 0 && errno == EBADF);
    gchar *original_path = g_file_get_path(original);
    descriptor = open(original_path, O_RDONLY);
    g_free(original_path);
    g_assert(descriptor >= 0 && !fdopendir(descriptor) && errno == ENOTDIR);
    g_assert(fcntl(descriptor, F_GETFD) >= 0 && !close(descriptor));
    async_files(directory, original);
    file_monitors(directory);
    g_assert(g_file_delete(original, NULL, &error));
    g_assert_no_error(error);
    g_assert(g_file_delete(moved, NULL, &error));
    g_assert_no_error(error);
    g_assert(g_file_delete(directory, NULL, &error));
    g_assert_no_error(error);
    g_object_unref(original);
    g_object_unref(copy);
    g_object_unref(moved);
    g_object_unref(directory);
    puts("GIO_FILES_COPY_MOVE_PASS");

    int descriptors[2];
    g_assert(socketpair(AF_UNIX, SOCK_STREAM, 0, descriptors) == 0);
    GSocket *sender = g_socket_new_from_fd(descriptors[0], &error);
    g_assert_no_error(error);
    GSocket *receiver = g_socket_new_from_fd(descriptors[1], &error);
    g_assert_no_error(error);
    g_assert(g_socket_send(sender, "café", 5, NULL, &error) == 5);
    g_assert_no_error(error);
    gchar buffer[16];
    g_assert(g_socket_receive(receiver, buffer, sizeof(buffer), NULL, &error) == 5);
    g_assert_no_error(error);
    g_assert(!memcmp(buffer, "café", 5));
    g_assert(g_socket_close(sender, &error));
    g_assert_no_error(error);
    g_assert(g_socket_receive(receiver, buffer, sizeof(buffer), NULL, &error) == 0);
    g_assert_no_error(error);
    g_object_unref(sender);
    g_object_unref(receiver);
    puts("GIO_UNIX_SOCKETS_PASS");

    int identity_pair[2];
    g_assert(socketpair(AF_UNIX, SOCK_STREAM, 0, identity_pair) == 0);
    GSocket *identity_socket = g_socket_new_from_fd(identity_pair[0], &error);
    g_assert_no_error(error);
    GCredentials *credentials = g_socket_get_credentials(identity_socket, &error);
    g_assert_no_error(error);
    g_assert(credentials && g_credentials_get_unix_user(credentials, &error) == geteuid());
    g_assert_no_error(error);
    g_assert(g_credentials_get_unix_pid(credentials, &error) == -1);
    g_assert_error(error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
    g_clear_error(&error);
    GCredentials *identity_copy = g_credentials_new();
    g_credentials_set_native(
        identity_copy, G_CREDENTIALS_TYPE_LUME_UID_GID,
        g_credentials_get_native(credentials, G_CREDENTIALS_TYPE_LUME_UID_GID));
    g_assert(g_credentials_is_same_user(credentials, identity_copy, &error));
    g_assert_no_error(error);
    g_assert(!g_credentials_set_unix_user(identity_copy, 1234, &error));
    g_assert_error(error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED);
    g_clear_error(&error);
    gchar *identity = g_credentials_to_string(identity_copy);
    g_assert(g_str_has_prefix(identity, "GCredentials:lume-uid-gid:"));
    g_free(identity);
    g_object_unref(identity_copy);
    g_object_unref(credentials);
    g_object_unref(identity_socket);
    g_assert(!close(identity_pair[1]));
    puts("GIO_PEER_CREDENTIALS_PASS");

    GResolver *resolver = g_resolver_get_default();
    GList *addresses = g_resolver_lookup_by_name(resolver, "localhost", NULL, &error);
    g_assert_no_error(error);
    g_assert(addresses != NULL);
    g_resolver_free_addresses(addresses);
    addresses = g_resolver_lookup_by_name(resolver, "127.1", NULL, &error);
    g_assert(addresses == NULL && error != NULL);
    g_clear_error(&error);
    g_object_unref(resolver);
    puts("GIO_RESOLVER_NUMERIC_PASS");

    GCancellable *cancellable = g_cancellable_new();
    g_cancellable_cancel(cancellable);
    g_assert(g_cancellable_is_cancelled(cancellable));
    g_cancellable_reset(cancellable);
    g_assert(!g_cancellable_is_cancelled(cancellable));
    GTask *task = g_task_new(NULL, cancellable, NULL, NULL);
    g_task_set_task_data(task, task, NULL);
    g_task_run_in_thread_sync(task, work);
    g_assert(g_task_propagate_int(task, &error) == 42);
    g_assert_no_error(error);
    g_object_unref(task);
    g_object_unref(cancellable);
    puts("GIO_ASYNC_TASK_PASS");
    puts("GIO_NATIVE_PASS");
    return 0;
}
