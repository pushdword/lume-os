/* SPDX-License-Identifier: BSD-3-Clause */
#include <glib.h>
#include <glib-object.h>
#include <glib/gstdio.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <locale.h>
#include <string.h>
#include <unistd.h>

static GRWLock shared_lock;
static gint writer_started, written;
static GPrivate thread_text = G_PRIVATE_INIT(g_free);
static _Alignas(64) _Thread_local unsigned char tls_zero[257];
static _Thread_local unsigned tls_number = 17;

static gpointer writer(gpointer argument)
{
    g_assert(((uintptr_t)tls_zero & 63) == 0 && tls_number == 17);
    for (unsigned i = 0; i < sizeof(tls_zero); ++i)
        g_assert(tls_zero[i] == 0);
    tls_zero[256] = 42;
    tls_number = 73;
    g_private_set(&thread_text, g_strdup("ação"));
    g_atomic_int_set(&writer_started, 1);
    g_rw_lock_writer_lock(&shared_lock);
    g_assert_cmpstr(g_private_get(&thread_text), ==, "ação");
    g_atomic_int_set(&written, 42);
    g_rw_lock_writer_unlock(&shared_lock);
    return argument;
}

static gint changed(GObject *object, const gchar *text, gdouble number, gpointer argument)
{
    g_assert(G_IS_OBJECT(object));
    g_assert_cmpstr(text, ==, "café");
    g_assert(number == 1.25);
    g_assert(argument == &written);
    return 73;
}

struct delivery
{
    GMainLoop *loop;
    GThread *receiver;
    gint count;
};

static gboolean receive(gpointer argument)
{
    struct delivery *delivery = argument;
    g_assert(g_thread_self() == delivery->receiver);
    ++delivery->count;
    g_main_loop_quit(delivery->loop);
    return G_SOURCE_REMOVE;
}

static gpointer notify(gpointer argument)
{
    struct delivery *delivery = argument;
    g_usleep(20000);
    GSource *source = g_idle_source_new();
    g_source_set_callback(source, receive, delivery, NULL);
    g_source_attach(source, g_main_loop_get_context(delivery->loop));
    g_source_unref(source);
    return NULL;
}

int main(void)
{
    g_assert(fileno(stdin) == 0 && fileno(stdout) == 1 && fileno(stderr) == 2);
    GError *conversion_error = NULL;
    gsize converted_length;
    char *converted = g_convert_with_fallback("prefix ação suffix", -1, "ASCII", "UTF-8", "?",
                                              NULL, &converted_length, &conversion_error);
    g_assert_no_error(conversion_error);
    g_assert_cmpstr(converted, ==, "prefix a??o suffix");
    g_assert(converted_length == strlen("prefix a??o suffix"));
    g_free(converted);
    g_assert(setlocale(LC_ALL, "C.UTF-8"));
    const GLogField fields[] = {{"MESSAGE", "GLIB_DIAGNOSTIC ação", -1}};
    char *diagnostic = g_log_writer_format_fields(G_LOG_LEVEL_MESSAGE, fields, G_N_ELEMENTS(fields), FALSE);
    g_assert(diagnostic && strstr(diagnostic, "GLIB_DIAGNOSTIC ação"));
    g_free(diagnostic);
    g_assert(g_log_writer_standard_streams(G_LOG_LEVEL_MESSAGE, fields, G_N_ELEMENTS(fields), NULL) ==
             G_LOG_WRITER_HANDLED);
    g_assert(((uintptr_t)tls_zero & 63) == 0 && tls_number == 17);
    assert(glib_major_version == 2 && glib_minor_version == 90);
    gchar *text = g_utf8_normalize("ac\xcc\xa7"
                                   "a\xcc\x83"
                                   "o",
                                   -1, G_NORMALIZE_NFC);
    g_assert_cmpstr(text, ==, "ação");
    g_free(text);
    g_assert(g_ascii_strtod("1.25", NULL) == 1.25);
    text = g_strdup_printf("%2$d %3$d %1$d", 1, 2, 3);
    g_assert_cmpstr(text, ==, "2 3 1");
    g_free(text);
    GError *error = NULL;
    GRegex *expression = g_regex_new("^caf\\p{L}+$", G_REGEX_OPTIMIZE, 0, &error);
    g_assert_no_error(error);
    g_assert(expression && g_regex_match(expression, "café", 0, NULL));
    g_regex_unref(expression);
    GHashTable *table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    g_hash_table_insert(table, g_strdup("café"), g_strdup("ação"));
    g_assert_cmpstr(g_hash_table_lookup(table, "café"), ==, "ação");
    g_hash_table_unref(table);
    GVariant *value =
        g_variant_ref_sink(g_variant_new("(st)", "ação", G_GUINT64_CONSTANT(0x100000001)));
    const gchar *label;
    guint64 number;
    g_variant_get(value, "(&st)", &label, &number);
    g_assert_cmpstr(label, ==, "ação");
    g_assert(number == G_GUINT64_CONSTANT(0x100000001));
    g_variant_unref(value);
    puts("GLIB_UNICODE_DATA_PASS");

    gchar *directory = g_strdup_printf("/data/glib-check-%ld", (long)getpid());
    gchar *path = g_build_filename(directory, "ação.ini", NULL);
    g_assert(g_mkdir_with_parents(directory, 0700) == 0);
    GKeyFile *settings = g_key_file_new();
    g_key_file_set_string(settings, "teste", "nome", "café");
    gsize length;
    gchar *contents = g_key_file_to_data(settings, &length, &error);
    g_assert_no_error(error);
    g_assert(g_file_set_contents(path, contents, (gssize)length, &error));
    g_assert_no_error(error);
    g_free(contents);
    g_assert(g_file_get_contents(path, &contents, &length, &error));
    g_assert_no_error(error);
    g_assert(g_key_file_load_from_data(settings, contents, length, G_KEY_FILE_NONE, &error));
    g_assert_no_error(error);
    text = g_key_file_get_string(settings, "teste", "nome", &error);
    g_assert_no_error(error);
    g_assert_cmpstr(text, ==, "café");
    g_free(text);
    GMappedFile *mapping = g_mapped_file_new(path, TRUE, &error);
    g_assert_no_error(error);
    g_assert(mapping && g_mapped_file_get_length(mapping) == length);
    g_assert(!memcmp(g_mapped_file_get_contents(mapping), contents, length));
    g_mapped_file_get_contents(mapping)[0] = '!';
    GBytes *snapshot = g_mapped_file_get_bytes(mapping);
    g_mapped_file_unref(mapping);
    g_assert(g_bytes_get_size(snapshot) == length);
    g_assert(((const char *)g_bytes_get_data(snapshot, NULL))[0] == '!');
    gchar *unchanged;
    gsize unchanged_length;
    g_assert(g_file_get_contents(path, &unchanged, &unchanged_length, &error));
    g_assert_no_error(error);
    g_assert(unchanged_length == length && !memcmp(unchanged, contents, length));
    g_free(unchanged);
    g_bytes_unref(snapshot);
    int descriptor = open(path, O_RDONLY);
    g_assert(descriptor >= 0 && lseek(descriptor, 2, SEEK_SET) == 2);
    g_assert(!g_mapped_file_new_from_fd(descriptor, FALSE, &error) && error);
    g_clear_error(&error);
    g_assert(lseek(descriptor, 0, SEEK_CUR) == 2 && !close(descriptor));
    puts("GLIB_PRIVATE_RESOURCE_SNAPSHOT_PASS");
    g_free(contents);
    g_key_file_unref(settings);
    g_assert(g_unlink(path) == 0 && g_rmdir(directory) == 0);
    g_free(path);
    g_free(directory);
    puts("GLIB_FILES_PASS");

    g_rw_lock_init(&shared_lock);
    g_rw_lock_reader_lock(&shared_lock);
    g_rw_lock_reader_lock(&shared_lock);
    GThread *thread = g_thread_new("writer", writer, &shared_lock);
    while (!g_atomic_int_get(&writer_started))
        g_usleep(1000);
    g_assert(!g_rw_lock_writer_trylock(&shared_lock));
    g_rw_lock_reader_unlock(&shared_lock);
    g_usleep(2000);
    g_assert(!g_atomic_int_get(&written));
    g_rw_lock_reader_unlock(&shared_lock);
    g_assert(g_thread_join(thread) == &shared_lock);
    g_assert(tls_zero[256] == 0 && tls_number == 17);
    g_assert(g_atomic_int_get(&written) == 42);
    g_rw_lock_writer_lock(&shared_lock);
    g_assert(!g_rw_lock_reader_trylock(&shared_lock));
    g_assert(!g_rw_lock_writer_trylock(&shared_lock));
    g_rw_lock_writer_unlock(&shared_lock);
    g_rw_lock_clear(&shared_lock);
    puts("GLIB_THREADS_RWLOCK_PASS");

    GObject *object = g_object_new(G_TYPE_OBJECT, NULL);
    guint signal =
        g_signal_new("changed", G_TYPE_OBJECT, G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                     g_cclosure_marshal_generic, G_TYPE_INT, 2, G_TYPE_STRING, G_TYPE_DOUBLE);
    g_assert(signal);
    g_signal_connect(object, "changed", G_CALLBACK(changed), &written);
    gint result = 0;
    g_signal_emit(object, signal, 0, "café", 1.25, &result);
    g_assert(result == 73);
    g_object_unref(object);
    puts("GOBJECT_SIGNAL_FFI_PASS");

    struct delivery delivery = {.loop = g_main_loop_new(NULL, FALSE), .receiver = g_thread_self()};
    thread = g_thread_new("notify", notify, &delivery);
    gint64 begin = g_get_monotonic_time();
    g_main_loop_run(delivery.loop);
    g_assert(delivery.count == 1 && g_get_monotonic_time() - begin >= 10000);
    g_thread_join(thread);
    g_main_loop_unref(delivery.loop);
    puts("GLIB_MAIN_LOOP_WAKE_PASS");
    puts("GLIB_NATIVE_PASS");
    return 0;
}
