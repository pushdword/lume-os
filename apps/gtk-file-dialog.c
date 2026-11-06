/* SPDX-License-Identifier: BSD-3-Clause */
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <lume/file_dialog.h>

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

struct input
{
    const unsigned char *bytes;
    size_t count;
    size_t position;
};

static int get_u32(struct input *input, uint32_t *value)
{
    if (input->count - input->position < 4)
        return 0;
    const unsigned char *p = input->bytes + input->position;
    *value = (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
    input->position += 4;
    return 1;
}

static char *get_string(struct input *input, size_t limit)
{
    uint32_t length;
    if (!get_u32(input, &length) || length > limit || length > input->count - input->position)
        return NULL;
    const char *start = (const char *)input->bytes + input->position;
    if (memchr(start, 0, length) || !g_utf8_validate(start, length, NULL))
        return NULL;
    input->position += length;
    return g_strndup(start, length);
}

static void put_u32(unsigned char *bytes, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i)
        bytes[i] = (unsigned char)(value >> (i * 8));
}

static int write_all(const unsigned char *bytes, size_t count)
{
    while (count)
    {
        ssize_t result = write(STDOUT_FILENO, bytes, count);
        if (result < 0 && errno == EINTR)
            continue;
        if (result <= 0)
            return 0;
        bytes += result;
        count -= (size_t)result;
    }
    return 1;
}

static unsigned char *read_request(size_t *count)
{
    unsigned char *bytes = g_malloc(LUME_FILE_DIALOG_REQUEST_LIMIT + 1);
    *count = 0;
    while (*count <= LUME_FILE_DIALOG_REQUEST_LIMIT)
    {
        ssize_t result =
            read(STDIN_FILENO, bytes + *count, LUME_FILE_DIALOG_REQUEST_LIMIT + 1 - *count);
        if (result < 0 && errno == EINTR)
            continue;
        if (result < 0)
            break;
        if (!result && *count <= LUME_FILE_DIALOG_REQUEST_LIMIT)
            return bytes;
        *count += (size_t)result;
    }
    g_free(bytes);
    return NULL;
}

static int emit_selection(GSList *files, uint32_t filter_index)
{
    size_t count = g_slist_length(files);
    if (count > LUME_FILE_DIALOG_SELECTION_LIMIT)
        return 0;
    GByteArray *bytes = g_byte_array_sized_new(16);
    unsigned char header[16];
    put_u32(header, LUME_FILE_DIALOG_RESPONSE_MAGIC);
    put_u32(header + 4, count ? 1 : 0);
    put_u32(header + 8, filter_index);
    put_u32(header + 12, (uint32_t)count);
    g_byte_array_append(bytes, header, sizeof(header));
    int valid = 1;
    for (GSList *item = files; item; item = item->next)
    {
        const char *path = item->data;
        size_t length = strlen(path);
        if (!g_path_is_absolute(path) || length > LUME_FILE_DIALOG_PATH_LIMIT ||
            !g_utf8_validate(path, length, NULL) ||
            bytes->len + 4 + length > LUME_FILE_DIALOG_RESPONSE_LIMIT)
        {
            valid = 0;
            break;
        }
        unsigned char prefix[4];
        put_u32(prefix, (uint32_t)length);
        g_byte_array_append(bytes, prefix, sizeof(prefix));
        g_byte_array_append(bytes, (const unsigned char *)path, length);
    }
    int result = valid && write_all(bytes->data, bytes->len);
    g_byte_array_unref(bytes);
    return result;
}

static int extension_valid(const char *extension)
{
    return !strpbrk(extension, "/\\*?[]");
}

static char *save_filename(GtkFileChooser *chooser, const char *extension, int *added)
{
    *added = 0;
    char *filename = gtk_file_chooser_get_filename(chooser);
    if (!filename || !extension[0])
        return filename;
    char *basename = g_path_get_basename(filename);
    int has_extension = strchr(basename, '.') != NULL;
    g_free(basename);
    if (has_extension)
        return filename;
    *added = 1;
    char *result = g_strconcat(filename, ".", extension, NULL);
    g_free(filename);
    return result;
}

static int confirm_replacement(GtkWindow *parent, const char *path)
{
    if (!g_file_test(path, G_FILE_TEST_EXISTS))
        return 1;
    GtkWidget *question = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
                                                 GTK_BUTTONS_YES_NO, "Replace the existing file?");
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(question), "%s", path);
    int accepted = gtk_dialog_run(GTK_DIALOG(question)) == GTK_RESPONSE_YES;
    gtk_widget_destroy(question);
    return accepted;
}

static int valid_data_filename(const char *name)
{
    size_t length = strlen(name);
    if (!length || length > NAME_MAX || name[length - 1] == '.' || name[length - 1] == ' ')
        return 0;
    for (const unsigned char *cursor = (const unsigned char *)name; *cursor; ++cursor)
    {
        if (*cursor < 32 || *cursor == 127 || strchr("\\:*?\"<>|", *cursor))
            return 0;
    }
    return 1;
}

static int validate_save_filename(GtkWindow *parent, GtkFileChooser *chooser, const char *path)
{
    char *directory = g_path_get_dirname(path);
    char *resolved = realpath(directory, NULL);
    int error = errno ? errno : EIO;
    g_free(directory);
    struct stat info;
    int accessible = resolved != NULL;
    if (resolved)
    {
        if (stat(resolved, &info) < 0)
        {
            error = errno;
            accessible = 0;
        }
        else if (!S_ISDIR(info.st_mode))
        {
            error = ENOTDIR;
            accessible = 0;
        }
    }
    if (!accessible)
    {
        GtkWidget *message = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
                                                    GTK_BUTTONS_CLOSE, "Unable to use this folder");
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(message), "%s",
                                                 strerror(error));
        gtk_dialog_run(GTK_DIALOG(message));
        gtk_widget_destroy(message);
        free(resolved);
        return 0;
    }
    int data_volume = !strcmp(resolved, "/data") || !strncmp(resolved, "/data/", 6);
    free(resolved);
    if (!data_volume)
        return 1;
    char *leaf = g_path_get_basename(path);
    char *entered = gtk_file_chooser_get_current_name(chooser);
    const char *entered_leaf = entered ? strrchr(entered, '/') : NULL;
    entered_leaf = entered_leaf ? entered_leaf + 1 : entered;
    /* The entry preserves trailing spaces which GTK trims when forming a path. */
    int valid = valid_data_filename(leaf) && entered_leaf && valid_data_filename(entered_leaf);
    g_free(entered);
    g_free(leaf);
    if (!valid)
    {
        GtkWidget *message =
            gtk_message_dialog_new(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                   "The filename is not supported by this volume");
        gtk_message_dialog_format_secondary_text(
            GTK_MESSAGE_DIALOG(message),
            "Use at most %u bytes. Avoid control characters and \\ : * ? \" < > |. "
            "The name cannot end in a dot or space.",
            (unsigned)NAME_MAX);
        gtk_dialog_run(GTK_DIALOG(message));
        gtk_widget_destroy(message);
    }
    return valid;
}

static int run_dialog(void)
{
    size_t count;
    unsigned char *bytes = read_request(&count);
    if (!bytes)
        return 2;
    struct input input = {bytes, count, 0};
    uint32_t magic, action, selected_filter, all_files, filter_count, owner_low, owner_high;
    if (!get_u32(&input, &magic) || magic != LUME_FILE_DIALOG_REQUEST_MAGIC ||
        !get_u32(&input, &action) || action < LUME_FILE_DIALOG_FOLDER ||
        action > LUME_FILE_DIALOG_MULTIPLE || !get_u32(&input, &selected_filter) ||
        !get_u32(&input, &all_files) || all_files > 1 || !get_u32(&input, &filter_count) ||
        filter_count > LUME_FILE_DIALOG_FILTER_LIMIT || !get_u32(&input, &owner_low) ||
        !get_u32(&input, &owner_high))
    {
        g_free(bytes);
        return 2;
    }
    char *title = get_string(&input, 4096);
    char *path = get_string(&input, LUME_FILE_DIALOG_PATH_LIMIT);
    char *extension = get_string(&input, 255);
    if (!title || !path || !extension || !extension_valid(extension))
    {
        g_free(title);
        g_free(path);
        g_free(extension);
        g_free(bytes);
        return 2;
    }
    GtkFileChooserAction gtk_action = action == LUME_FILE_DIALOG_SAVE ? GTK_FILE_CHOOSER_ACTION_SAVE
                                      : action <= LUME_FILE_DIALOG_EXISTING_FOLDER
                                          ? GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER
                                          : GTK_FILE_CHOOSER_ACTION_OPEN;
    const char *accept = action == LUME_FILE_DIALOG_SAVE ? "_Save" : "_Select";
    GtkWidget *dialog =
        gtk_file_chooser_dialog_new(title[0] ? title : "Select a file", NULL, gtk_action, "_Cancel",
                                    GTK_RESPONSE_CANCEL, accept, GTK_RESPONSE_ACCEPT, NULL);
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_file_chooser_set_local_only(chooser, TRUE);
    gtk_file_chooser_set_select_multiple(chooser, action == LUME_FILE_DIALOG_MULTIPLE);
    gtk_file_chooser_set_create_folders(chooser, action != LUME_FILE_DIALOG_EXISTING_FOLDER);
    gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
    if (path[0])
    {
        if (g_file_test(path, G_FILE_TEST_IS_DIR))
        {
            gtk_file_chooser_set_current_folder(chooser, path);
        }
        else if (action == LUME_FILE_DIALOG_SAVE)
        {
            char *directory = g_path_get_dirname(path);
            char *name = g_path_get_basename(path);
            gtk_file_chooser_set_current_folder(chooser, directory);
            gtk_file_chooser_set_current_name(chooser, name);
            g_free(directory);
            g_free(name);
        }
        else
        {
            gtk_file_chooser_set_filename(chooser, path);
        }
    }
    GPtrArray *filters = g_ptr_array_new();
    int valid = 1;
    for (uint32_t i = 0; valid && i < filter_count; ++i)
    {
        char *description = get_string(&input, 4096);
        uint32_t patterns;
        if (!description || !get_u32(&input, &patterns) || !patterns ||
            patterns > LUME_FILE_DIALOG_PATTERN_LIMIT)
        {
            g_free(description);
            valid = 0;
            break;
        }
        GtkFileFilter *filter = gtk_file_filter_new();
        gtk_file_filter_set_name(filter, description[0] ? description : "Files");
        g_free(description);
        for (uint32_t n = 0; n < patterns; ++n)
        {
            char *suffix = get_string(&input, 255);
            if (!suffix || !suffix[0] || !extension_valid(suffix))
            {
                g_free(suffix);
                valid = 0;
                break;
            }
            char *pattern = g_strconcat("*.", suffix, NULL);
            gtk_file_filter_add_pattern(filter, pattern);
            g_free(pattern);
            g_free(suffix);
        }
        gtk_file_chooser_add_filter(chooser, filter);
        g_ptr_array_add(filters, filter);
    }
    if (all_files || !filter_count)
    {
        GtkFileFilter *filter = gtk_file_filter_new();
        gtk_file_filter_set_name(filter, "All files");
        gtk_file_filter_add_pattern(filter, "*");
        gtk_file_chooser_add_filter(chooser, filter);
        g_ptr_array_add(filters, filter);
    }
    if (selected_filter && selected_filter <= filters->len)
        gtk_file_chooser_set_filter(chooser, g_ptr_array_index(filters, selected_filter - 1));
    uint64_t owner = (uint64_t)owner_low | (uint64_t)owner_high << 32;
    if (owner)
    {
        gtk_widget_realize(dialog);
        GdkWindow *parent =
            gdk_x11_window_foreign_new_for_display(gtk_widget_get_display(dialog), (Window)owner);
        if (parent)
        {
            gdk_window_set_transient_for(gtk_widget_get_window(dialog), parent);
            g_object_unref(parent);
        }
    }
    valid = valid && input.position == input.count;
    GSList *files = NULL;
    uint32_t index = 0;
    while (valid && gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        if (action == LUME_FILE_DIALOG_SAVE)
        {
            int extension_added;
            char *filename = save_filename(chooser, extension, &extension_added);
            if (!filename)
                break;
            if (!validate_save_filename(GTK_WINDOW(dialog), chooser, filename))
            {
                g_free(filename);
                continue;
            }
            if (extension_added && !confirm_replacement(GTK_WINDOW(dialog), filename))
            {
                g_free(filename);
                continue;
            }
            files = g_slist_append(NULL, filename);
        }
        else
        {
            files = gtk_file_chooser_get_filenames(chooser);
        }
        GtkFileFilter *filter = gtk_file_chooser_get_filter(chooser);
        for (guint i = 0; i < filters->len; ++i)
        {
            if ((filter_count || all_files) && filter == g_ptr_array_index(filters, i))
                index = i + 1;
        }
        break;
    }
    int result = valid && emit_selection(files, index);
    g_slist_free_full(files, g_free);
    g_ptr_array_free(filters, TRUE);
    gtk_widget_destroy(dialog);
    g_free(extension);
    g_free(path);
    g_free(title);
    g_free(bytes);
    return result ? 0 : 2;
}

static int open_item(const char *argument, int file)
{
    GError *error = NULL;
    char *uri = file ? g_filename_to_uri(argument, NULL, &error) : g_strdup(argument);
    int result = uri && g_app_info_launch_default_for_uri(uri, NULL, &error);
    if (!result)
        fprintf(stderr, "Unable to open item: %s\n", error ? error->message : "invalid URI");
    g_clear_error(&error);
    g_free(uri);
    return result ? 0 : 2;
}

int main(int argc, char **argv)
{
    if (argc == 3 && !strcmp(argv[1], "--open-file"))
        return open_item(argv[2], 1);
    if (argc == 3 && !strcmp(argv[1], "--open-uri"))
        return open_item(argv[2], 0);
    if (argc != 1 || !gtk_init_check(&argc, &argv))
    {
        fprintf(stderr, "Unable to initialize file dialog\n");
        return 2;
    }
    int result = run_dialog();
    if (result)
        fprintf(stderr, "Unable to complete file dialog\n");
    return result;
}
