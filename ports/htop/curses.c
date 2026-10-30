/* SPDX-License-Identifier: BSD-3-Clause */
#include "curses.h"
#include "api.h"
#include "fd_client.h"
#include <stdio.h>
#include <string.h>
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
#include <termios.h>
#endif
static WINDOW window;
WINDOW *stdscr = &window;
int COLS = 80, LINES = 24, COLORS = 8, COLOR_PAIRS = 256;
static chtype wanted[128 * 48], shown[128 * 48], attributes;
static short foreground[256], background[256];
static int delay_ms = 1000, force = 1, dirty = 1, cursor_visible = 1;
static int console_input = 1, input_eof;
static struct lume_stat input_metadata;
static struct lume_pollfd input_poll;
static struct lume_fd_request input_request;
char *enter_ca_mode = "", *exit_ca_mode = "", *clear_screen = "\033[2J\033[H";

static unsigned now(void)
{
    return (unsigned)lume_syscall(LUME_INFO, LUME_UPTIME, 0, 0);
}

static int out(const char *s, unsigned n)
{
    while (n)
    {
        unsigned take = n > 4096 ? 4096 : n;
        int k = app_write(1, s, take);
        if (k == -4)
            continue;
        if (k <= 0 || (unsigned)k > take)
            return ERR;
        s += k;
        n -= (unsigned)k;
    }
    return OK;
}

WINDOW *initscr(void)
{
    int metadata = lume_syscall(LUME_FSTAT, 0, (uint32_t)(uintptr_t)&input_metadata, 0);
    console_input = metadata < 0 || (input_metadata.mode & 0170000) == 0020000;
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
    struct termios mode;
    if (!tcgetattr(0, &mode))
        console_input = 0;
#endif
    input_eof = 0;
    int g = console_input ? lume_syscall(LUME_TERMINAL, 0, 0, 0) : (24 << 16) | 80;
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
    if (!tcgetattr(0, &mode))
        g = lume_syscall(LUME_TERMINAL, 0, 0, 0);
#endif
    int c = g & 65535, l = (unsigned)g >> 16;
    if (c > 0 && c <= 128 && l > 0 && l <= 48)
    {
        COLS = c;
        LINES = l;
    }
    lume_stdio_raw(1);
    for (int i = 0; i < 256; i++)
    {
        foreground[i] = 7;
        background[i] = 0;
    }
    attributes = 0;
    clear();
    return stdscr;
}

int endwin(void)
{
    int rc = out("\033[0m\033[?25h", 10);
    cursor_visible = 1;
    lume_stdio_raw(0);
    return rc;
}

int move(int y, int x)
{
    if (y < 0 || y >= LINES || x < 0 || x >= COLS)
        return ERR;
    window.row = y;
    window.column = x;
    return OK;
}

int addch(chtype c)
{
    if (window.row < 0 || window.row >= LINES || window.column < 0 || window.column >= COLS)
        return ERR;
    unsigned at = (unsigned)(window.row * COLS + window.column);
    chtype value = c | attributes;
    if (wanted[at] != value)
    {
        wanted[at] = value;
        dirty = 1;
    }
    if (++window.column >= COLS)
    {
        window.column = 0;
        if (window.row + 1 < LINES)
            window.row++;
    }
    return OK;
}

int addnstr(const char *s, int n)
{
    if (n < 0)
        n = 0x7fffffff;
    for (int i = 0; s[i] && i < n; i++)
        addch((unsigned char)s[i]);
    return OK;
}

int addstr(const char *s)
{
    return addnstr(s, -1);
}

int mvaddch(int y, int x, chtype c)
{
    return move(y, x) == ERR ? ERR : addch(c);
}

int mvaddnstr(int y, int x, const char *s, int n)
{
    return move(y, x) == ERR ? ERR : addnstr(s, n);
}

int mvaddstr(int y, int x, const char *s)
{
    return mvaddnstr(y, x, s, -1);
}

int mvaddchnstr(int y, int x, const chtype *s, int n)
{
    if (move(y, x) == ERR)
        return ERR;
    if (n < 0)
        n = 0x7fffffff;
    for (int i = 0; s[i] && i < n; i++)
    {
        chtype saved = attributes;
        attributes = 0;
        addch(s[i]);
        attributes = saved;
    }
    return OK;
}

int mvaddchstr(int y, int x, const chtype *s)
{
    return mvaddchnstr(y, x, s, -1);
}

int clear(void)
{
    for (int i = 0; i < COLS * LINES; i++)
        wanted[i] = ' ';
    window.row = window.column = 0;
    force = dirty = 1;
    return OK;
}

int erase(void)
{
    return clear();
}

int clrtoeol(void)
{
    int end = (window.row + 1) * COLS;
    for (int i = window.row * COLS + window.column; i < end; i++)
    {
        chtype value = ' ' | attributes;
        if (wanted[i] != value)
        {
            wanted[i] = value;
            dirty = 1;
        }
    }
    return OK;
}

int attrset(int a)
{
    attributes = (chtype)a;
    return OK;
}

int attron(int a)
{
    attributes |= (chtype)a;
    return OK;
}

int attroff(int a)
{
    attributes &= ~(chtype)a;
    return OK;
}

int mvhline(int y, int x, chtype c, int n)
{
    for (int i = 0; i < n && x + i < COLS; i++)
        mvaddch(y, x + i, c ? c : '-');
    return OK;
}

int mvvline(int y, int x, chtype c, int n)
{
    for (int i = 0; i < n && y + i < LINES; i++)
        mvaddch(y + i, x, c ? c : '|');
    return OK;
}

int init_pair(short p, short f, short b)
{
    if (p < 0 || p >= 256)
        return ERR;
    foreground[p] = f;
    background[p] = b;
    return OK;
}

int pair_content(short p, short *f, short *b)
{
    if (p < 0 || p >= 256)
        return ERR;
    *f = foreground[p];
    *b = background[p];
    return OK;
}

/* Only publish shown[] after every write succeeded. A partial terminal write
 * forces a complete redraw on retry. Batches amortize the kernel trap/flush. */
static char output[4096];
static unsigned output_used;

static int append(const char *s, unsigned n)
{
    while (n)
    {
        unsigned room = sizeof output - output_used;
        if (!room)
        {
            if (out(output, output_used) == ERR)
                return ERR;
            output_used = 0;
            room = sizeof output;
        }
        unsigned take = n < room ? n : room;
        memcpy(output + output_used, s, take);
        output_used += take;
        s += take;
        n -= take;
    }
    return OK;
}

/* CUP/SGR use only small unsigned decimal fields. Avoid the full varargs
 * printf formatter for every changed span of every frame. */
static unsigned decimal(char *s, unsigned value)
{
    unsigned n = 0;
    if (value >= 100)
    {
        s[n++] = (char)('0' + value / 100);
        value %= 100;
    }
    if (value >= 10 || n)
    {
        s[n++] = (char)('0' + value / 10);
        value %= 10;
    }
    s[n++] = (char)('0' + value);
    return n;
}

static unsigned cup(char *s, unsigned y, unsigned x)
{
    unsigned n = 0;
    s[n++] = '\033';
    s[n++] = '[';
    n += decimal(s + n, y);
    s[n++] = ';';
    n += decimal(s + n, x);
    s[n++] = 'H';
    return n;
}

static unsigned sgr(char *s, chtype attr)
{
    unsigned pair = PAIR_NUMBER(attr);
    int f = foreground[pair], b = background[pair];
    unsigned fg = (unsigned)(f < 0   ? 39
                             : f < 8 ? 30 + f
                                     : 90 + (f - 8) % 8),
             bg = (unsigned)(b < 0   ? 49
                             : b < 8 ? 40 + b
                                     : 100 + (b - 8) % 8);
    unsigned n = 0;
    memcpy(s, "\033[0;", 4);
    n = 4;
    n += decimal(s + n, fg);
    s[n++] = ';';
    n += decimal(s + n, bg);
    if (attr & A_BOLD)
    {
        s[n++] = ';';
        s[n++] = '1';
    }
    if (attr & A_DIM)
    {
        s[n++] = ';';
        s[n++] = '2';
    }
    if (attr & A_REVERSE)
    {
        s[n++] = ';';
        s[n++] = '7';
    }
    s[n++] = 'm';
    return n;
}

int refresh(void)
{
    if (!dirty && !force)
        return OK;
    char text[128];
    output_used = 0;
    chtype active_attr = 0;
    int have_attr = 0;
    for (int y = 0; y < LINES; y++)
        for (int x = 0; x < COLS;)
        {
            int at = y * COLS + x;
            if (!force && wanted[at] == shown[at])
            {
                x++;
                continue;
            }
            chtype attr = wanted[at] & ~255u;
            unsigned n = cup(text, (unsigned)y + 1, (unsigned)x + 1);
            if (!have_attr || active_attr != attr)
            {
                n += sgr(text + n, attr);
                active_attr = attr;
                have_attr = 1;
            }
            if (append(text, n) == ERR)
                goto fail;
            unsigned count = 0;
            do
            {
                chtype c = wanted[y * COLS + x];
                text[count++] = (char)((c & 255u) >= 32 && (c & 255u) < 127 ? (c & 255u) : ' ');
                x++;
            } while (x < COLS && (wanted[y * COLS + x] & ~255u) == attr &&
                     (force || wanted[y * COLS + x] != shown[y * COLS + x]));
            if (append(text, count) == ERR)
                goto fail;
        }
    if (output_used && out(output, output_used) == ERR)
        goto fail;
    memcpy(shown, wanted, (unsigned)(COLS * LINES) * sizeof *shown);
    force = dirty = 0;
    return OK;
fail:
    force = dirty = 1;
    output_used = 0;
    return ERR;
}

bool has_colors(void)
{
    return true;
}

int start_color(void)
{
    return OK;
}

int use_default_colors(void)
{
    return OK;
}

int noecho(void)
{
    return OK;
}

int cbreak(void)
{
    return OK;
}

int nocbreak(void)
{
    return OK;
}

int nonl(void)
{
    return OK;
}

int intrflush(WINDOW *w, bool b)
{
    (void)w;
    (void)b;
    return OK;
}

int keypad(WINDOW *w, bool b)
{
    (void)w;
    (void)b;
    return OK;
}

int halfdelay(int n)
{
    delay_ms = n * 100;
    return OK;
}

int nodelay(WINDOW *w, bool b)
{
    (void)w;
    delay_ms = b ? 0 : -1;
    return OK;
}

void timeout(int n)
{
    delay_ms = n;
}

int napms(int ms)
{
    while (ms > 0)
    {
        unsigned n = ms > 60000 ? 60000 : (unsigned)ms;
        lume_syscall(LUME_SLEEP, n, 0, 0);
        ms -= (int)n;
    }
    return OK;
}

int curs_set(int n)
{
    if (n < 0 || n > 2)
        return ERR;
    int old = cursor_visible;
    if ((n != 0) == old)
        return old;
    if (n && mvcur(-1, -1, window.row, window.column) == ERR)
        return ERR;
    if (out(n ? "\033[?25h" : "\033[?25l", 6) == ERR)
        return ERR;
    cursor_visible = n != 0;
    return old;
}

int beep(void)
{
    return out("\a", 1);
}

int define_key(const char *s, int n)
{
    (void)s;
    (void)n;
    return ERR;
}

int set_escdelay(int n)
{
    (void)n;
    return OK;
}

int mvcur(int oldy, int oldx, int y, int x)
{
    (void)oldy;
    (void)oldx;
    if (move(y, x) == ERR)
        return ERR;
    char text[32];
    unsigned n = cup(text, (unsigned)y + 1, (unsigned)x + 1);
    return out(text, n);
}

int tputs(const char *s, int affected, int (*put)(int))
{
    (void)affected;
    if (s)
        while (*s)
            put((unsigned char)*s++);
    return OK;
}

static int poll_input(unsigned timeout)
{
    input_request = app_fd_request();
    input_request.buffer = lume_fd_pointer(&input_poll);
    input_request.count = 1;
    input_request.timeout_ms = timeout;
    return app_fd_call(LUME_FD_POLL, &input_request);
}

static int byte(int ms)
{
    unsigned start = now();
    do
    {
        input_poll = (struct lume_pollfd){.fd = 0, .events = LUME_POLLIN};
        int ready = console_input ? lume_syscall(LUME_TERMINAL, 1, 0, 0) : poll_input(0);
        if (ready < 0)
            return ERR;
        if (ready > 0)
        {
            unsigned char c;
            int n = app_read(0, &c, 1);
            if (n == 1)
                return c;
            if (!n || n == -32 || n == -9)
                input_eof = 1;
            if (n != -4)
                return ERR;
        }
        if (ms == 0)
            return ERR;
        unsigned wait = 60000;
        if (ms > 0)
        {
            unsigned elapsed = now() - start;
            if (elapsed >= (unsigned)ms)
                return ERR;
            wait = (unsigned)ms - elapsed;
        }
        /* The existing ABI can park this process until input or timeout. Keep the
  * polling fallback for kernels/controllers without that optional service. */
        int rc = console_input ? lume_syscall(LUME_TERMINAL, 5, wait, 0) : poll_input(wait);
        if (rc == -95 || rc == -38 || rc == -22)
        {
            if (wait > 50)
                wait = 50;
            if (lume_syscall(LUME_SLEEP, wait, 0, 0) < 0)
                return ERR;
        }
        else if (rc < 0 && rc != -11)
            return ERR;
    } while (1);
}

int getch(void)
{
    int g = console_input ? lume_syscall(LUME_TERMINAL, 0, 0, 0) : (LINES << 16) | COLS;
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
    struct termios mode;
    if (!tcgetattr(0, &mode))
        g = lume_syscall(LUME_TERMINAL, 0, 0, 0);
#endif
    int cols = g & 65535, lines = (unsigned)g >> 16;
    if (cols > 0 && cols <= 128 && lines > 0 && lines <= 48 && (cols != COLS || lines != LINES))
    {
        COLS = cols;
        LINES = lines;
        clear();
        return KEY_RESIZE;
    } /* curses getch publishes modified stdscr before waiting for input. */
    if ((dirty || force) && refresh() == ERR)
        return ERR;
    if (cursor_visible && mvcur(-1, -1, window.row, window.column) == ERR)
        return ERR;
    int c = byte(delay_ms);
    if (input_eof)
        return 'q';
    if (c == 127 || c == 8)
        return KEY_BACKSPACE;
    if (c == '\r' || c == '\n')
        return KEY_ENTER;
    if (c == 3)
        return 'q';
    if (c != 27)
        return c;
    int n = byte(50);
    if (n == ERR)
        return 27;
    if (n != '[' && n != 'O')
        return n;
    int value = 0;
    for (int i = 0; i < 12; i++)
    {
        c = byte(50);
        if (c == ERR)
            return 27;
        if (c >= '0' && c <= '9')
        {
            value = value * 10 + c - '0';
            continue;
        }
        switch (c)
        {
        case 'A':
            return KEY_UP;
        case 'B':
            return KEY_DOWN;
        case 'C':
            return KEY_RIGHT;
        case 'D':
            return KEY_LEFT;
        case 'H':
            return KEY_HOME;
        case 'F':
            return KEY_END;
        case 'P':
            return KEY_F(1);
        case 'Q':
            return KEY_F(2);
        case 'R':
            return KEY_F(3);
        case 'S':
            return KEY_F(4);
        case '~':
            return value == 1                   ? KEY_HOME
                   : value == 4                 ? KEY_END
                   : value == 3                 ? KEY_DC
                   : value == 5                 ? KEY_PPAGE
                   : value == 6                 ? KEY_NPAGE
                   : value >= 11 && value <= 15 ? KEY_F(value - 10)
                   : value >= 17 && value <= 21 ? KEY_F(value - 11)
                                                : 27;
        default:
            return 27;
        }
    }
    return 27;
}
