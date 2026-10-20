/* SPDX-License-Identifier: BSD-3-Clause
 * Bounded ASCII curses-compatible terminal adapter for original htop UI.
 * This is not ncurses; only the interfaces used by this port are implemented. */
#ifndef LUME_HTOP_CURSES_H
#define LUME_HTOP_CURSES_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
typedef uint32_t chtype;
typedef uint32_t attr_t;

typedef struct
{
    int row, column;
} WINDOW;

extern WINDOW *stdscr;
extern int COLS, LINES, COLORS, COLOR_PAIRS;
#define TRUE 1
#define FALSE 0
#define OK 0
#define ERR (-1)
#define A_NORMAL 0u
#define A_BOLD 0x100u
#define A_DIM 0x200u
#define A_REVERSE 0x400u
#define A_CHARTEXT 0xffu
#define A_COLOR 0xff0000u
#define COLOR_PAIR(n) ((chtype)(n) << 16)
#define PAIR_NUMBER(n) (((n) >> 16) & 255u)
#define COLOR_BLACK 0
#define COLOR_RED 1
#define COLOR_GREEN 2
#define COLOR_YELLOW 3
#define COLOR_BLUE 4
#define COLOR_MAGENTA 5
#define COLOR_CYAN 6
#define COLOR_WHITE 7
#define KEY_DOWN 0x102
#define KEY_UP 0x103
#define KEY_LEFT 0x104
#define KEY_RIGHT 0x105
#define KEY_HOME 0x106
#define KEY_BACKSPACE 0x107
#define KEY_F(n) (0x110 + (n))
#define KEY_DC 0x14a
#define KEY_NPAGE 0x152
#define KEY_PPAGE 0x153
#define KEY_ENTER 0x157
#define KEY_END 0x168
#define KEY_SLEFT 0x189
#define KEY_SRIGHT 0x192
#define KEY_SF 0x150
#define KEY_SR 0x151
#define KEY_RESIZE 0x19a
#define KEY_MOUSE 0x199
#define KEY_MAX 0x1ff
#define ACS_VLINE '|'
#define ACS_HLINE '-'
#define ACS_LTEE '+'
#define ACS_LLCORNER '+'
#define ACS_ULCORNER '+'
#define ACS_URCORNER '+'
#define ACS_LRCORNER '+'
WINDOW *initscr(void);
int endwin(void), refresh(void), clear(void), erase(void), clrtoeol(void);
int move(int, int), addch(chtype), addstr(const char *), addnstr(const char *, int);
int mvaddch(int, int, chtype), mvaddstr(int, int, const char *),
    mvaddnstr(int, int, const char *, int);
int mvaddchnstr(int, int, const chtype *, int), mvaddchstr(int, int, const chtype *);
int mvhline(int, int, chtype, int), mvvline(int, int, chtype, int);
int attrset(int), attron(int), attroff(int), curs_set(int), beep(void);
int init_pair(short, short, short), pair_content(short, short *, short *);
bool has_colors(void);
int start_color(void), use_default_colors(void), noecho(void), cbreak(void), nocbreak(void),
    nonl(void);
int intrflush(WINDOW *, bool), keypad(WINDOW *, bool), halfdelay(int), nodelay(WINDOW *, bool);
void timeout(int);
int getch(void), napms(int), define_key(const char *, int), set_escdelay(int),
    mvcur(int, int, int, int);
#define getmaxyx(w, y, x)                                                                          \
    do                                                                                             \
    {                                                                                              \
        (void)(w);                                                                                 \
        (y) = LINES;                                                                               \
        (x) = COLS;                                                                                \
    } while (0)
#define getyx(w, y, x)                                                                             \
    do                                                                                             \
    {                                                                                              \
        (y) = (w)->row;                                                                            \
        (x) = (w)->column;                                                                         \
    } while (0)
#endif
