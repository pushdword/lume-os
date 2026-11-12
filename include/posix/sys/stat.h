/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_POSIX_SYS_STAT_H
#define LUME_POSIX_SYS_STAT_H
#include_next <sys/stat.h>

#if defined(_PICOLIBC__) && __LARGEFILE64_VISIBLE &&                                               \
    (defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686))
#ifdef __cplusplus
extern "C"
{
#endif
    int fstatat64(int, const char *, struct stat64 *, int);
    int lstat64(const char *, struct stat64 *);
#ifdef __cplusplus
}
#endif

/* The large-file interface also preserves native 64-bit inode identities. */
#if !defined(LUME_STAT_IMPLEMENTATION) &&                                                          \
    (defined(LUME_NATIVE_LFS64) || defined(LUME_NATIVE_CXX) ||                                     \
     (defined(_FILE_OFFSET_BITS) && _FILE_OFFSET_BITS == 64))
#define LUME_POSIX_STAT64 1
#define stat stat64
#define fstat fstat64
#define lstat lstat64
#define fstatat fstatat64
#endif
#endif
#endif
