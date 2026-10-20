/* SPDX-License-Identifier: GPL-2.0-only
 * Selected BusyBox 1.38.0 libbb interface for LumeOS. */
#ifndef LUME_LIBBB_H
#define LUME_LIBBB_H
#define _DEFAULT_SOURCE 1
#include "lume_libc.h"
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
#define getopt32long(argv,spec,longopts,...) getopt32(argv,spec, ##__VA_ARGS__)
#define FILEUTILS_RECUR 1
#define FILEUTILS_VERBOSE 2
#define LOGMODE_STDIO 1
#define LONE_CHAR(s,c) ((s)[0]==(c)&&!(s)[1])
#define NOT_LONE_CHAR(s,c) (!LONE_CHAR(s,c))
#define OFF_FMT "l"
typedef unsigned exitcode_t;
typedef unsigned long uoff_t;
typedef struct llist_t {struct llist_t *link;char *data;} llist_t;
typedef signed char smallint;
extern char **environ;
extern const char *applet_name;
extern const char bb_msg_write_error[],bb_msg_standard_input[];
extern const char *const bb_argv_dash[];
struct suffix_mult {const char *suffix;unsigned mult;};
extern const struct suffix_mult bkm_suffixes[];
_Noreturn void bb_show_usage(void);
void bb_error_msg(const char *,...);
void bb_simple_perror_msg(const char *);
void *xmalloc(size_t);
ssize_t safe_write(int,const void *,size_t);
ssize_t full_write(int,const void *,size_t);
char *single_argv(char **);
char *last_char_is(const char *,int);
char *bb_get_last_path_component_strip(char *);
char bb_process_escape_sequence(const char **);
unsigned getopt32(char **,const char *,...);
extern unsigned option_mask32;
extern int xfunc_error_retval,logmode;
void *xrealloc(void *,size_t);
void *xzalloc(size_t);
void *lume_vector(void *,size_t,unsigned,unsigned);
#define xrealloc_vector(v,s,i) lume_vector(v,sizeof(*(v)),s,i)
FILE *xfopen_stdin(const char *);
char *xmalloc_fgets(FILE *);
char *xmalloc_fgetline(FILE *);
void fputs_stdout(const char *);
void die_if_ferror(FILE *,const char *);
_Noreturn void bb_error_msg_and_die(const char *,...);
_Noreturn void bb_simple_perror_msg_and_die(const char *);
#define bb_simple_error_msg_and_die(s) bb_error_msg_and_die("%s",s)
void bb_perror_msg(const char *,...);
unsigned xatou_range(const char *,unsigned,unsigned);
int xatoi_positive(const char *);
#define xatoull_sfx(s,t) ((unsigned long long)xatoul_sfx(s,t))
#define kmg_i_suffixes bkm_suffixes
int bb_make_directory(char *,long,int);
mode_t bb_parse_mode(const char *,mode_t);
uint32_t *crc32_filltable(uint32_t *,int);
uint32_t crc32_block_endian1(uint32_t,const void *,unsigned,uint32_t *);
uint32_t crc32_block_endian0(uint32_t,const void *,unsigned,uint32_t *);
ssize_t safe_read(int,void *,size_t);
#define fflush_stdout_and_exit_SUCCESS() fflush_stdout_and_exit(0)
int open_or_warn_stdin(const char *);
FILE *fopen_or_warn_stdin(const char *);
int fclose_if_not_stdin(FILE *);
off_t bb_copyfd_eof(int,int);
int bb_cat(char **);
unsigned long xatoul_sfx(const char *,const struct suffix_mult *);
char *xrealloc_getcwd_or_warn(char *);
int fflush_all(void);
void die_if_ferror_stdout(void);
_Noreturn void fflush_stdout_and_exit(int);
#define isprint_asciionly(c) ((unsigned)((c)-32)<=94)
#endif
