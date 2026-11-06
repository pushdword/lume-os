#ifndef LUME_POSIX_DIRENT_H
#define LUME_POSIX_DIRENT_H
#include <sys/types.h>
#include <stdint.h>
#if defined(LUME_NATIVE_ABI) || defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) ||          \
    defined(LUME_ARCH_I686)
#include <lume/files.h>
#define LUME_POSIX_DIRECTORY_NATIVE 1
#endif

#define DT_UNKNOWN 0
#define DT_FIFO 1
#define DT_CHR 2
#define DT_DIR 4
#define DT_BLK 6
#define DT_REG 8
#define DT_LNK 10
#define DT_SOCK 12

struct dirent
{
    ino_t d_ino;
    unsigned short d_reclen;
    unsigned char d_type;
#ifdef LUME_POSIX_DIRECTORY_NATIVE
    char d_name[LUME_FILES_NAME_MAX + 1];
#else
    char d_name[48];
#endif
};

#ifdef LUME_POSIX_DIRECTORY_NATIVE
typedef struct
{
    int fd;
    unsigned count, next, eof;
    uint64_t position;
    unsigned char buffer[4 * sizeof(struct lume_files_dirent)];
    struct dirent entry;
} DIR;
#else
typedef struct
{
    char path[128];
    unsigned index;
    struct dirent entry;
} DIR;
#endif
#endif
