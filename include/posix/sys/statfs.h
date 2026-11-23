/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_POSIX_SYS_STATFS_H
#define LUME_POSIX_SYS_STATFS_H
#include <sys/types.h>

struct statfs
{
    unsigned long f_type;
    unsigned long f_bsize;
    fsblkcnt_t f_blocks;
    fsblkcnt_t f_bfree;
    fsblkcnt_t f_bavail;
    fsfilcnt_t f_files;
    fsfilcnt_t f_ffree;
    unsigned long f_fsid;
    unsigned long f_namelen;
    unsigned long f_frsize;
    unsigned long f_flags;
};

#ifdef __cplusplus
extern "C"
{
#endif
    int statfs(const char *, struct statfs *);
    int fstatfs(int, struct statfs *);
#ifdef __cplusplus
}
#endif

#endif
