#ifndef LUME_POSIX_DIRENT_H
#define LUME_POSIX_DIRENT_H
#include <sys/types.h>
#include <stdint.h>

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
    char d_name[48];
};

#if defined(LUME_NATIVE_ABI) || defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64)
typedef struct
{
    int fd;
    unsigned count, next, eof;
    uint64_t position;
    unsigned char buffer[1024];
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
