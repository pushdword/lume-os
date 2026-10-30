/* SPDX-License-Identifier: GPL-2.0-only
 * Selected upstream vi profile, separate ELF, shared selected libbb services. */
#include "../busybox/libbb.h"
#include <stddef.h>
/* Adapter types used by the vi port. */
typedef unsigned nfds_t;

struct pollfd
{
    int fd;
    short events, revents;
};

#define POLLIN 1

#if defined(LUME_ARCH_AMD64)
#include <termios.h>
#else
struct termios
{
    unsigned char c_cc[32];
};

#define VERASE 2
#endif
#define CONFIG_FEATURE_VI_MAX_LEN 256
#define ENABLE_FEATURE_VI_8BIT 0
#define ENABLE_FEATURE_VI_COLON 1
#define ENABLE_FEATURE_VI_COLON_EXPAND 0
#define ENABLE_FEATURE_VI_YANKMARK 1
#define ENABLE_FEATURE_VI_SEARCH 1
#define ENABLE_FEATURE_VI_REGEX_SEARCH 0
#define ENABLE_FEATURE_VI_USE_SIGNALS 0
#define ENABLE_FEATURE_VI_DOT_CMD 0
#define ENABLE_FEATURE_VI_READONLY 1
#define ENABLE_FEATURE_VI_SETOPTS 1
#define ENABLE_FEATURE_VI_SET 1
#define ENABLE_FEATURE_VI_WIN_RESIZE 1
#define ENABLE_FEATURE_VI_ASK_TERMINAL 0
#define ENABLE_FEATURE_VI_UNDO 1
#define ENABLE_FEATURE_VI_UNDO_QUEUE 0
#define ENABLE_FEATURE_VI_VERBOSE_STATUS 1
#define ENABLE_FEATURE_ALLOW_EXEC 0
#define IF_FEATURE_VI_COLON(...) __VA_ARGS__
#define IF_FEATURE_VI_SEARCH(...) __VA_ARGS__
#define IF_FEATURE_VI_SETOPTS(...) __VA_ARGS__
#define IF_FEATURE_VI_READONLY(...) __VA_ARGS__
#define IF_FEATURE_VI_DOT_CMD(...)
#define IF_FEATURE_VI_COLON_EXPAND(...)
#define IF_FEATURE_VI_ASK_TERMINAL(...)
#define ALWAYS_INLINE inline __attribute__((always_inline))
#define NOINLINE __attribute__((noinline))
#define TRUE 1
#define FALSE 0
#define KEYCODE_BUFFER_SIZE 16
#define TERMIOS_RAW_CRNL 1
#define SET_PTR_TO_GLOBALS(p) (ptr_to_globals = (p))
#define bb_putchar(c) putchar(c)
#define bb_strtou(s, e, b) ((unsigned)strtoul(s, e, b))
struct globals;
extern struct globals *ptr_to_globals;
typedef unsigned char smalluint;
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define STRERROR_FMT "%s"
#define STRERROR_ERRNO , strerror(errno)
#define BB_VER "BusyBox1.38.0 / LumeOS vi"
void *memrchr(const void *, int, size_t);
char *strchrnul(const char *, int);
char *skip_whitespace(const char *);
char *skip_non_whitespace(const char *);
int index_in_strings(const char *, const char *);
void *llist_pop(llist_t **);

enum
{
    KEYCODE_UP = -2,
    KEYCODE_DOWN = -3,
    KEYCODE_LEFT = -4,
    KEYCODE_RIGHT = -5,
    KEYCODE_HOME = -6,
    KEYCODE_END = -7,
    KEYCODE_PAGEUP = -8,
    KEYCODE_PAGEDOWN = -9,
    KEYCODE_DELETE = -10,
    KEYCODE_INSERT = -11,
    KEYCODE_FUN1 = -20,
    KEYCODE_FUN2 = -21,
    KEYCODE_FUN3 = -22,
    KEYCODE_FUN4 = -23,
    KEYCODE_FUN5 = -24,
    KEYCODE_FUN6 = -25,
    KEYCODE_FUN7 = -26,
    KEYCODE_FUN8 = -27,
    KEYCODE_FUN9 = -28,
    KEYCODE_FUN10 = -29,
    KEYCODE_FUN11 = -30,
    KEYCODE_FUN12 = -31
};

int get_terminal_width_height(int, unsigned *, unsigned *);
int safe_poll(struct pollfd *, nfds_t, int);
int64_t safe_read_key(int, char *, int);
void set_termios_to_raw(int, struct termios *, int);
void tcsetattr_stdin_TCSANOW(const struct termios *);
ssize_t full_read(int, void *, size_t);
char *xstrdup(const char *);
char *xstrndup(const char *, size_t);
char *concat_path_file(const char *, const char *);
char *xmalloc_open_read_close(const char *, size_t *);
char *safe_strncpy(char *, const char *, size_t);
