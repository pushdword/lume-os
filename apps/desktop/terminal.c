/* SPDX-License-Identifier: BSD-3-Clause */
#include <gtk/gtk.h>
#include <glib-unix.h>
#include <vterm.h>
#include <errno.h>
#include <fcntl.h>
#include <pty.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

struct terminal
{
    GtkWidget *window, *view;
    PangoLayout *layout;
    PangoFontDescription *font;
    VTerm *engine;
    VTermScreen *screen;
    GByteArray *input;
    int fd, rows, columns, cell_width, cell_height;
    guint reader, writer, child_watch;
    pid_t child;
    char title[256];
    size_t title_bytes;
};

static gboolean write_input(gint fd, GIOCondition condition, gpointer data)
{
    struct terminal *term = data;
    while (term->input->len && !(condition & (G_IO_HUP | G_IO_ERR)))
    {
        ssize_t count = write(fd, term->input->data, term->input->len);
        if (count > 0)
            g_byte_array_remove_range(term->input, 0, count);
        else if (count < 0 && errno == EINTR)
            continue;
        else if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            return G_SOURCE_CONTINUE;
        else
            break;
    }
    term->writer = 0;
    return G_SOURCE_REMOVE;
}

static void output(const char *bytes, size_t length, void *data)
{
    struct terminal *term = data;
    if (length > 1024 * 1024 - term->input->len)
    {
        gtk_widget_error_bell(term->view);
        return;
    }
    g_byte_array_append(term->input, (const guint8 *)bytes, length);
    if (!term->writer && !write_input(term->fd, 0, term))
        return;
    if (!term->writer)
        term->writer = g_unix_fd_add(term->fd, G_IO_OUT | G_IO_HUP | G_IO_ERR, write_input, term);
}

static int damage(VTermRect rectangle, void *data)
{
    struct terminal *term = data;
    gtk_widget_queue_draw_area(term->view, 0, 4 + rectangle.start_row * term->cell_height,
                               8 + term->columns * term->cell_width,
                               (rectangle.end_row - rectangle.start_row) * term->cell_height);
    return 1;
}

static int cursor(VTermPos pos, VTermPos old, int visible, void *data)
{
    (void)visible;
    struct terminal *term = data;
    gtk_widget_queue_draw_area(term->view, 4 + old.col * term->cell_width,
                               4 + old.row * term->cell_height, term->cell_width,
                               term->cell_height);
    gtk_widget_queue_draw_area(term->view, 4 + pos.col * term->cell_width,
                               4 + pos.row * term->cell_height, term->cell_width,
                               term->cell_height);
    return 1;
}

static int property(VTermProp property, VTermValue *value, void *data)
{
    struct terminal *term = data;
    if (property == VTERM_PROP_TITLE)
    {
        if (value->string.initial)
            term->title_bytes = 0;
        size_t bytes = MIN(value->string.len, sizeof(term->title) - 1 - term->title_bytes);
        memcpy(term->title + term->title_bytes, value->string.str, bytes);
        term->title_bytes += bytes;
        term->title[term->title_bytes] = 0;
        if (value->string.final)
        {
            char *title = g_utf8_make_valid(term->title, term->title_bytes);
            gtk_window_set_title(GTK_WINDOW(term->window), title);
            g_free(title);
        }
    }
    return 1;
}

static const VTermScreenCallbacks callbacks = {
    .damage = damage, .movecursor = cursor, .settermprop = property};

static gboolean read_output(gint fd, GIOCondition condition, gpointer data)
{
    struct terminal *term = data;
    char buffer[16384];
    for (unsigned batch = 0; batch < 4; ++batch)
    {
        ssize_t count = read(fd, buffer, sizeof(buffer));
        if (count > 0)
            vterm_input_write(term->engine, buffer, count);
        else if (count < 0 && errno == EINTR)
            continue;
        else if (!count || (count < 0 && errno != EAGAIN && errno != EWOULDBLOCK))
        {
            term->reader = 0;
            gtk_main_quit();
            return G_SOURCE_REMOVE;
        }
        else
            break;
    }
    vterm_screen_flush_damage(term->screen);
    if (condition & (G_IO_HUP | G_IO_ERR))
    {
        term->reader = 0;
        gtk_main_quit();
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}

static void source_color(cairo_t *cr, VTermColor color)
{
    cairo_set_source_rgb(cr, color.rgb.red / 255.0, color.rgb.green / 255.0,
                         color.rgb.blue / 255.0);
}

static gboolean draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    (void)widget;
    struct terminal *term = data;
    cairo_set_source_rgb(cr, 0.04, 0.06, 0.09);
    cairo_paint(cr);
    double x1, y1, x2, y2;
    cairo_clip_extents(cr, &x1, &y1, &x2, &y2);
    int first = MAX(0, (int)(y1 - 4) / term->cell_height);
    int last = MIN(term->rows, (int)(y2 - 4) / term->cell_height + 1);
    for (int row = first; row < last; ++row)
        for (int column = 0; column < term->columns;)
        {
            VTermScreenCell cell, next;
            vterm_screen_get_cell(term->screen, (VTermPos){row, column}, &cell);
            if (cell.width < 1)
            {
                ++column;
                continue;
            }
            VTermColor foreground = cell.fg, background = cell.bg;
            vterm_screen_convert_color_to_rgb(term->screen, &foreground);
            vterm_screen_convert_color_to_rgb(term->screen, &background);
            if (cell.attrs.reverse)
            {
                VTermColor swap = foreground;
                foreground = background;
                background = swap;
            }
            char text[4096];
            int used = 0, start = column;
            do
            {
                next = cell;
                if (!cell.chars[0])
                    text[used++] = ' ';
                else
                    for (unsigned i = 0; i < VTERM_MAX_CHARS_PER_CELL && cell.chars[i]; ++i)
                        used += g_unichar_to_utf8(cell.chars[i], text + used);
                column += MAX(1, cell.width);
                if (column >= term->columns ||
                    used > (int)sizeof(text) - VTERM_MAX_CHARS_PER_CELL * 6)
                    break;
                vterm_screen_get_cell(term->screen, (VTermPos){row, column}, &cell);
            } while (cell.width > 0 && next.attrs.bold == cell.attrs.bold &&
                     next.attrs.italic == cell.attrs.italic &&
                     next.attrs.underline == cell.attrs.underline &&
                     next.attrs.reverse == cell.attrs.reverse &&
                     vterm_color_is_equal(&next.fg, &cell.fg) &&
                     vterm_color_is_equal(&next.bg, &cell.bg));
            int x = 4 + start * term->cell_width, y = 4 + row * term->cell_height;
            int width = (column - start) * term->cell_width;
            source_color(cr, background);
            cairo_rectangle(cr, x, y, width, term->cell_height);
            cairo_fill(cr);
            pango_font_description_set_weight(term->font, next.attrs.bold ? PANGO_WEIGHT_BOLD
                                                                          : PANGO_WEIGHT_NORMAL);
            pango_font_description_set_style(term->font, next.attrs.italic ? PANGO_STYLE_ITALIC
                                                                           : PANGO_STYLE_NORMAL);
            pango_layout_set_font_description(term->layout, term->font);
            pango_layout_set_text(term->layout, text, used);
            source_color(cr, foreground);
            cairo_move_to(cr, x, y);
            pango_cairo_show_layout(cr, term->layout);
            if (next.attrs.underline)
            {
                cairo_rectangle(cr, x, y + term->cell_height - 2, width, 1);
                cairo_fill(cr);
            }
        }
    VTermPos pos;
    vterm_state_get_cursorpos(vterm_obtain_state(term->engine), &pos);
    cairo_set_source_rgb(cr, 0.85, 0.85, 0.85);
    cairo_rectangle(cr, 4.5 + pos.col * term->cell_width, 4.5 + pos.row * term->cell_height,
                    term->cell_width - 1, term->cell_height - 1);
    cairo_set_line_width(cr, 1);
    cairo_stroke(cr);
    return FALSE;
}

static void resize(GtkWidget *widget, GtkAllocation *allocation, gpointer data)
{
    (void)widget;
    struct terminal *term = data;
    int rows = CLAMP((allocation->height - 8) / term->cell_height, 1, 500);
    int columns = CLAMP((allocation->width - 8) / term->cell_width, 1, 1000);
    if (rows == term->rows && columns == term->columns)
        return;
    term->rows = rows;
    term->columns = columns;
    vterm_set_size(term->engine, rows, columns);
    struct winsize size = {.ws_row = rows, .ws_col = columns};
    ioctl(term->fd, TIOCSWINSZ, &size);
    vterm_screen_flush_damage(term->screen);
}

static void paste(GtkClipboard *clipboard, const gchar *text, gpointer data)
{
    (void)clipboard;
    struct terminal *term = data;
    if (!text || strlen(text) > 256 * 1024 || !g_utf8_validate(text, -1, NULL))
        return;
    vterm_keyboard_start_paste(term->engine);
    for (const char *p = text; *p; p = g_utf8_next_char(p))
        vterm_keyboard_unichar(term->engine, g_utf8_get_char(p), VTERM_MOD_NONE);
    vterm_keyboard_end_paste(term->engine);
}

static gboolean key(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    (void)widget;
    struct terminal *term = data;
    if ((event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) ==
            (GDK_CONTROL_MASK | GDK_SHIFT_MASK) &&
        (event->keyval == GDK_KEY_V || event->keyval == GDK_KEY_v))
    {
        gtk_clipboard_request_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), paste, term);
        return TRUE;
    }
    VTermKey key = VTERM_KEY_NONE;
    switch (event->keyval)
    {
    case GDK_KEY_Return:
        key = VTERM_KEY_ENTER;
        break;
    case GDK_KEY_Tab:
        key = VTERM_KEY_TAB;
        break;
    case GDK_KEY_BackSpace:
        key = VTERM_KEY_BACKSPACE;
        break;
    case GDK_KEY_Escape:
        key = VTERM_KEY_ESCAPE;
        break;
    case GDK_KEY_Up:
        key = VTERM_KEY_UP;
        break;
    case GDK_KEY_Down:
        key = VTERM_KEY_DOWN;
        break;
    case GDK_KEY_Left:
        key = VTERM_KEY_LEFT;
        break;
    case GDK_KEY_Right:
        key = VTERM_KEY_RIGHT;
        break;
    case GDK_KEY_Home:
        key = VTERM_KEY_HOME;
        break;
    case GDK_KEY_End:
        key = VTERM_KEY_END;
        break;
    case GDK_KEY_Page_Up:
        key = VTERM_KEY_PAGEUP;
        break;
    case GDK_KEY_Page_Down:
        key = VTERM_KEY_PAGEDOWN;
        break;
    case GDK_KEY_Insert:
        key = VTERM_KEY_INS;
        break;
    case GDK_KEY_Delete:
        key = VTERM_KEY_DEL;
        break;
    default:
        if (event->keyval >= GDK_KEY_F1 && event->keyval <= GDK_KEY_F35)
            key = VTERM_KEY_FUNCTION(event->keyval - GDK_KEY_F1 + 1);
        break;
    }
    VTermModifier modifiers = VTERM_MOD_NONE;
    if (event->state & GDK_SHIFT_MASK)
        modifiers |= VTERM_MOD_SHIFT;
    if (event->state & GDK_CONTROL_MASK)
        modifiers |= VTERM_MOD_CTRL;
    if (event->state & GDK_MOD1_MASK)
        modifiers |= VTERM_MOD_ALT;
    if (key != VTERM_KEY_NONE)
        vterm_keyboard_key(term->engine, key, modifiers);
    else
    {
        gunichar unicode = gdk_keyval_to_unicode(event->keyval);
        if (!unicode)
            return FALSE;
        vterm_keyboard_unichar(term->engine, unicode, modifiers);
    }
    return TRUE;
}

static void child_exited(GPid pid, gint status, gpointer data)
{
    (void)pid;
    (void)status;
    struct terminal *term = data;
    term->child = 0;
    term->child_watch = 0;
    gtk_main_quit();
}

int main(int argc, char **argv)
{
    if (argc >= 3 && !strcmp(argv[1], "--pty-child"))
    {
        if (ioctl(STDIN_FILENO, TIOCSCTTY, 0) || tcsetpgrp(STDIN_FILENO, getpgrp()))
        {
            perror("lume-terminal: controlling terminal");
            return 1;
        }
        execvp(argv[2], argv + 2);
        perror("lume-terminal: shell");
        return 1;
    }
    if (argc == 2 && !strcmp(argv[1], "--help"))
    {
        puts("Usage: lume-terminal [-e command [arguments...]]");
        return 0;
    }
    if (argc > 1 && (argc < 3 || strcmp(argv[1], "-e")))
        return 2;
    struct terminal term = {.rows = 24, .columns = 80, .fd = -1};
    if (!gtk_init_check(NULL, NULL))
        return 1;
    term.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    term.view = gtk_drawing_area_new();
    gtk_window_set_title(GTK_WINDOW(term.window), "Terminal");
    gtk_widget_set_can_focus(term.view, TRUE);
    gtk_container_add(GTK_CONTAINER(term.window), term.view);
    term.layout = gtk_widget_create_pango_layout(term.view, "M");
    term.font = pango_font_description_from_string("Monospace 12");
    pango_layout_set_font_description(term.layout, term.font);
    pango_layout_get_pixel_size(term.layout, &term.cell_width, &term.cell_height);
    term.cell_width = MAX(1, term.cell_width);
    term.cell_height = MAX(1, term.cell_height);
    term.engine = vterm_new(term.rows, term.columns);
    if (!term.engine)
        return 1;
    vterm_set_utf8(term.engine, 1);
    term.screen = vterm_obtain_screen(term.engine);
    vterm_screen_enable_altscreen(term.screen, 1);
    vterm_screen_set_callbacks(term.screen, &callbacks, &term);
    vterm_screen_set_damage_merge(term.screen, VTERM_DAMAGE_ROW);
    VTermColor foreground, background;
    vterm_color_rgb(&foreground, 225, 225, 225);
    vterm_color_rgb(&background, 10, 15, 23);
    vterm_screen_set_default_colors(term.screen, &foreground, &background);
    term.input = g_byte_array_new();
    int slave;
    struct winsize size = {.ws_row = term.rows, .ws_col = term.columns};
    if (openpty(&term.fd, &slave, NULL, NULL, &size))
    {
        perror("lume-terminal: pseudoterminal");
        return 1;
    }
    setenv("TERM", "xterm-256color", 1);
    const char *shell = access("/data/bin/bash", X_OK) == 0 ? "/data/bin/bash" : "/bin/bash";
    char *self = g_find_program_in_path(argv[0]);
    char **child = g_new0(char *, argc + 5);
    child[0] = self;
    child[1] = "--pty-child";
    if (argc > 2)
        for (int i = 2; i < argc; ++i)
            child[i] = argv[i];
    else
    {
        child[2] = (char *)shell;
        child[3] = "-i";
    }
    posix_spawn_file_actions_t actions;
    posix_spawnattr_t attributes;
    posix_spawn_file_actions_init(&actions);
    posix_spawnattr_init(&attributes);
    for (int fd = 0; fd < 3; ++fd)
        posix_spawn_file_actions_adddup2(&actions, slave, fd);
    posix_spawn_file_actions_addclose(&actions, term.fd);
    posix_spawn_file_actions_addclose(&actions, slave);
    posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETSID);
    int error =
        self ? posix_spawn(&term.child, self, &actions, &attributes, child, environ) : ENOENT;
    posix_spawnattr_destroy(&attributes);
    posix_spawn_file_actions_destroy(&actions);
    close(slave);
    g_free(child);
    g_free(self);
    if (error)
    {
        fprintf(stderr, "lume-terminal: cannot start shell: %s\n", strerror(error));
        close(term.fd);
        return 1;
    }
    fcntl(term.fd, F_SETFL, fcntl(term.fd, F_GETFL) | O_NONBLOCK);
    vterm_output_set_callback(term.engine, output, &term);
    vterm_screen_reset(term.screen, 1);
    term.reader = g_unix_fd_add(term.fd, G_IO_IN | G_IO_HUP | G_IO_ERR, read_output, &term);
    term.child_watch = g_child_watch_add(term.child, child_exited, &term);
    g_signal_connect(term.view, "draw", G_CALLBACK(draw), &term);
    g_signal_connect(term.view, "size-allocate", G_CALLBACK(resize), &term);
    g_signal_connect(term.window, "key-press-event", G_CALLBACK(key), &term);
    g_signal_connect(term.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_window_set_default_size(GTK_WINDOW(term.window), 8 + term.columns * term.cell_width,
                                8 + term.rows * term.cell_height);
    gtk_widget_show_all(term.window);
    gtk_widget_grab_focus(term.view);
    gtk_main();
    if (term.reader)
        g_source_remove(term.reader);
    if (term.writer)
        g_source_remove(term.writer);
    if (term.child_watch)
        g_source_remove(term.child_watch);
    close(term.fd);
    if (term.child)
    {
        kill(-term.child, SIGHUP);
        waitpid(term.child, NULL, WNOHANG);
    }
    vterm_free(term.engine);
    g_byte_array_unref(term.input);
    pango_font_description_free(term.font);
    g_object_unref(term.layout);
    return 0;
}
