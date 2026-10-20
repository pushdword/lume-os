#ifndef LUME_MINI_LIBC_H
#define LUME_MINI_LIBC_H
#include "lume/abi.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <limits.h>
typedef int64_t ssize_t;
typedef int64_t off_t;
typedef unsigned mode_t;
#define EOF (-1)
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#define O_RDONLY LUME_O_RDONLY
#define O_WRONLY LUME_O_WRONLY
#define O_RDWR LUME_O_RDWR
#define O_CREAT LUME_O_CREAT
#define O_TRUNC LUME_O_TRUNC
#define O_APPEND LUME_O_APPEND
#define EINTR 4
#define ENOMEM LUME_ENOMEM
#define ENOENT LUME_ENOENT
#define EINVAL LUME_EINVAL
#define EEXIST LUME_EEXIST
#define ENOSYS LUME_ENOSYS
#define ERANGE 34
#define SEEK_SET LUME_SEEK_SET
#define SEEK_CUR LUME_SEEK_CUR
#define SEEK_END LUME_SEEK_END
#define S_IFDIR 0040000u
#define S_IFREG 0100000u
#define S_ISDIR(m) (((m) & 0170000u) == S_IFDIR)

struct stat
{
    mode_t st_mode;
    off_t st_size;
    uint64_t st_ino, st_dev;
};

typedef struct
{
    int fd, error, eof, ungot, used;
} FILE;

extern FILE *stdin, *stdout, *stderr;
extern int errno, optind, opterr, optopt;
extern char *optarg, **environ;
int64_t lume_call(uint64_t, uint64_t, uint64_t, uint64_t);
_Noreturn void exit(int);
void *malloc(size_t);
void *realloc(void *, size_t);
void free(void *);
void *memcpy(void *, const void *, size_t);
void *memmove(void *, const void *, size_t);
void *memset(void *, int, size_t);
int memcmp(const void *, const void *, size_t);
size_t strlen(const char *);
int strcmp(const char *, const char *);
int strncmp(const char *, const char *, size_t);
char *strcpy(char *, const char *);
char *stpcpy(char *, const char *);
char *strchr(const char *, int);
char *strrchr(const char *, int);
char *strstr(const char *, const char *);
char *strdup(const char *);
char *strerror(int);
char *dirname(char *);
unsigned long strtoul(const char *, char **, int);
unsigned long long strtoull(const char *, char **, int);
int atoi(const char *);
int isspace(int);
int isdigit(int);
int isprint(int);
char *getenv(const char *);
char *getcwd(char *, size_t);
int open(const char *, int, ...);
int close(int);
ssize_t read(int, void *, size_t);
ssize_t write(int, const void *, size_t);
off_t lseek(int, off_t, int);
int stat(const char *, struct stat *);
int mkdir(const char *, mode_t);
int rmdir(const char *);
int unlink(const char *);
FILE *fopen(const char *, const char *);
int fclose(FILE *);
int getc(FILE *);
int fgetc(FILE *);
int ungetc(int, FILE *);
int ferror(FILE *);
int feof(FILE *);
void clearerr(FILE *);
int fputc(int, FILE *);
int putchar(int);
int fputs(const char *, FILE *);
int puts(const char *);
int fflush(FILE *);
size_t fread(void *, size_t, size_t, FILE *);
size_t fwrite(const void *, size_t, size_t, FILE *);
ssize_t getline(char **, size_t *, FILE *);
int vsnprintf(char *, size_t, const char *, va_list);
int snprintf(char *, size_t, const char *, ...);
int printf(const char *, ...);
int fprintf(FILE *, const char *, ...);
int vfprintf(FILE *, const char *, va_list);
int getopt(int, char *const *, const char *);
#endif
