/* SPDX-License-Identifier: GPL-2.0-only
 * Selected BusyBox 1.38.0 libbb interface for LumeOS. */
#ifndef LUME_LIBBB_H
#define LUME_LIBBB_H
#include "lume_features.h"
#define _DEFAULT_SOURCE 1
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>
#if defined(LUME_BUSYBOX_EXTENDED)
#include <dirent.h>
#include <time.h>
#include <sys/wait.h>
#include <spawn.h>
#define PUSH_AND_SET_FUNCTION_VISIBILITY_TO_HIDDEN
#define POP_SAVED_FUNCTION_VISIBILITY
#define TRUE 1
#define FALSE 0
#define SKIP 2
#define CURRENT_TTY "/dev/tty"
#define DOT_OR_DOTDOT(s) ((s)[0] == '.' && (!(s)[1] || ((s)[1] == '.' && !(s)[2])))
#define exit_SUCCESS() exit(0)
#define exit_FAILURE() exit(1)
#define bb_putchar_stderr(c) fputc((c), stderr)
#define BUILD_BUG_ON(condition) _Static_assert(!(condition), "invalid buffer size")
#define LONE_DASH(s) LONE_CHAR((s), '-')
#define ALIGN_SUFFIX
#define FIX_ALIASING __attribute__((may_alias))
#define SET_PTR_TO_GLOBALS(p) (ptr_to_globals = (p))
#define FREE_PTR_TO_GLOBALS() free(ptr_to_globals)
#define barrier() __asm__ volatile("" ::: "memory")
typedef unsigned char smalluint;
struct llist_t;
extern void *ptr_to_globals;

enum
{
    ACTION_RECURSE = 1 << 0,
    ACTION_FOLLOWLINKS = 1 << 1,
    ACTION_FOLLOWLINKS_L0 = 1 << 2,
    ACTION_DEPTHFIRST = 1 << 3,
    ACTION_QUIET = 1 << 4,
    ACTION_DANGLING_OK = 1 << 5
};

typedef uint8_t recurse_flags_t;

typedef struct recursive_state
{
    unsigned flags, depth;
    void *userData;
    int (*fileAction)(struct recursive_state *, const char *, struct stat *);
    int (*dirAction)(struct recursive_state *, const char *, struct stat *);
} recursive_state_t;

int recursive_action(const char *, unsigned,
                     int (*)(struct recursive_state *, const char *, struct stat *),
                     int (*)(struct recursive_state *, const char *, struct stat *), void *);
char *xstrdup(const char *);
char *xstrndup(const char *, size_t);
char *skip_whitespace(const char *);
char *skip_non_whitespace(const char *);
char *is_prefixed_with(const char *, const char *);
char *concat_path_file(const char *, const char *);
char *concat_subpath_file(const char *, const char *);
FILE *xfopen_for_write(const char *);
FILE *fopen_for_read(const char *);
void llist_free(struct llist_t *, void (*)(void *));
void llist_add_to(struct llist_t **, void *);
void *llist_pop(struct llist_t **);
int xopen(const char *, int, ...);
int xdup2(int, int);
void close_on_exec_on(int);
int spawn_and_wait(char **);
unsigned bb_arg_max(void);
unsigned count_strstr(const char *, const char *);
char *xmalloc_substitute_string(const char *, unsigned, const char *, const char *);
void overlapping_strcpy(char *, const char *);
char *safe_strncpy(char *, const char *, size_t);
const char *bb_basename(const char *);
char *xasprintf(const char *, ...);
char *xasprintf_and_free(char *, ...);
#define xasprintf_inplace(allocated, ...)                                                          \
    ((allocated) = xasprintf_and_free((allocated), __VA_ARGS__))
void llist_add_to_end(struct llist_t **, void *);
FILE *fopen_or_warn(const char *, const char *);
char *bb_get_chunk_from_file(FILE *, size_t *);
int index_in_strings(const char *, const char *);
unsigned long xatoul(const char *);
void xstat(const char *, struct stat *);
int xmkstemp(char *);
void xrename(const char *, const char *);
extern const char bb_msg_invalid_arg_to[], bb_msg_requires_arg[];
extern void (*die_func)(void);
FILE *xfdopen_for_write(int);
_Noreturn void bb_die_memory_exhausted(void);
#endif
#define FAST_FUNC
#define MAIN_EXTERNALLY_VISIBLE
#define UNUSED_PARAM __attribute__((unused))
#define ALIGN1 __attribute__((aligned(1)))
#define ENABLE_FEATURE_FANCY_ECHO 1
#define ENABLE_FEATURE_CATN 0
#define ENABLE_FEATURE_CATV 0
#define IF_FEATURE_CATN(...)
#define IF_FEATURE_CATV(...)
#define ENABLE_FEATURE_FANCY_HEAD 0
#define ENABLE_INCLUDE_SUSv2 0
#define ENABLE_DESKTOP 0
#define ENABLE_FEATURE_WC_LARGE 1
#define ENABLE_LOCALE_SUPPORT 0
#define ENABLE_FEATURE_CLEAN_UP 1
#define ENABLE_FEATURE_SORT_BIG 0
#define ENABLE_FEATURE_SORT_OPTIMIZE_MEMORY 0
#define ENABLE_FEATURE_CUT_REGEX 0
#define ENABLE_LONG_OPTS 0
#define ENABLE_FEATURE_VERBOSE 1
#define ENABLE_SELINUX 0
#define ENABLE_EXPAND 1
#define ENABLE_UNEXPAND 1
#define ENABLE_UNICODE_SUPPORT 0
#define ENABLE_CKSUM 1
#define ENABLE_CRC32 1
#define IF_LONG_OPTS(...)
#define IF_NOT_LONG_OPTS(...) __VA_ARGS__
#define IF_FEATURE_CUT_REGEX(...)
#define IF_NOT_FEATURE_CUT_REGEX(...) __VA_ARGS__
#define IF_FEATURE_VERBOSE(...) __VA_ARGS__
#define IF_SELINUX(...)
#define IF_DESKTOP(...)
#define IF_NOT_DESKTOP(...) __VA_ARGS__
#define IF_EXPAND(...) __VA_ARGS__
#define IF_UNEXPAND(...) __VA_ARGS__
#define IF_CKSUM(...) __VA_ARGS__
#define No_argument "\0"
#define Required_argument "\1"
#define getopt32long(argv, spec, longopts, ...) getopt32(argv, spec, ##__VA_ARGS__)
#define FILEUTILS_RECUR 1
#define FILEUTILS_VERBOSE 2
#define LOGMODE_STDIO 1
#define LONE_CHAR(s, c) ((s)[0] == (c) && !(s)[1])
#define NOT_LONE_CHAR(s, c) (!LONE_CHAR(s, c))
#define OFF_FMT "l"
typedef unsigned exitcode_t;
typedef unsigned long uoff_t;

typedef struct llist_t
{
    struct llist_t *link;
    char *data;
} llist_t;

typedef signed char smallint;
extern char **environ;
extern const char *applet_name;
extern const char bb_msg_write_error[], bb_msg_standard_input[];
extern const char *const bb_argv_dash[];

struct suffix_mult
{
    const char *suffix;
    unsigned mult;
};
extern const struct suffix_mult bkm_suffixes[];
_Noreturn void bb_show_usage(void);
void bb_error_msg(const char *, ...);
void bb_simple_perror_msg(const char *);
void *xmalloc(size_t);
ssize_t safe_write(int, const void *, size_t);
ssize_t full_write(int, const void *, size_t);
char *single_argv(char **);
char *last_char_is(const char *, int);
char *bb_get_last_path_component_strip(char *);
char bb_process_escape_sequence(const char **);
unsigned getopt32(char **, const char *, ...);
extern unsigned option_mask32;
extern int xfunc_error_retval, logmode;
void *xrealloc(void *, size_t);
void *xzalloc(size_t);
void *lume_vector(void *, size_t, unsigned, unsigned);
#define xrealloc_vector(v, s, i) lume_vector(v, sizeof(*(v)), s, i)
FILE *xfopen_stdin(const char *);
char *xmalloc_fgets(FILE *);
char *xmalloc_fgetline(FILE *);
void fputs_stdout(const char *);
void die_if_ferror(FILE *, const char *);
_Noreturn void bb_error_msg_and_die(const char *, ...);
_Noreturn void bb_simple_perror_msg_and_die(const char *);
#define bb_simple_error_msg_and_die(s) bb_error_msg_and_die("%s", s)
void bb_perror_msg(const char *, ...);
unsigned xatou_range(const char *, unsigned, unsigned);
int xatoi_positive(const char *);
#define xatoull_sfx(s, t) ((unsigned long long)xatoul_sfx(s, t))
#define kmg_i_suffixes bkm_suffixes
int bb_make_directory(char *, long, int);
mode_t bb_parse_mode(const char *, mode_t);
uint32_t *crc32_filltable(uint32_t *, int);
uint32_t crc32_block_endian1(uint32_t, const void *, unsigned, uint32_t *);
uint32_t crc32_block_endian0(uint32_t, const void *, unsigned, uint32_t *);
ssize_t safe_read(int, void *, size_t);
#define fflush_stdout_and_exit_SUCCESS() fflush_stdout_and_exit(0)
int open_or_warn_stdin(const char *);
FILE *fopen_or_warn_stdin(const char *);
int fclose_if_not_stdin(FILE *);
off_t bb_copyfd_eof(int, int);
int bb_cat(char **);
unsigned long xatoul_sfx(const char *, const struct suffix_mult *);
char *xrealloc_getcwd_or_warn(char *);
int fflush_all(void);
void die_if_ferror_stdout(void);
_Noreturn void fflush_stdout_and_exit(int);
#define isprint_asciionly(c) ((unsigned)((c) - 32) <= 94)
#endif
