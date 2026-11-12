/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_RUNTIME_STAT64_H
#define LUME_RUNTIME_STAT64_H
#include "files_client.h"
#include <fcntl.h>

#if defined(_PICOLIBC__) && __LARGEFILE64_VISIBLE &&                                               \
    (defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686))
int stat64(const char *path, struct stat64 *metadata)
{
    return lume_files_result(lume_native_stat64_at(LUME_FILES_AT_CWD, path, 0, metadata));
}

int fstat64(int fd, struct stat64 *metadata)
{
    return lume_files_result(lume_native_stat64_at(fd, "", LUME_FILES_STAT_EMPTY_PATH, metadata));
}

int lstat64(const char *path, struct stat64 *metadata)
{
    return lume_files_result(
        lume_native_stat64_at(LUME_FILES_AT_CWD, path, LUME_FILES_STAT_NOFOLLOW, metadata));
}

int fstatat64(int directory, const char *path, struct stat64 *metadata, int flags)
{
    unsigned allowed = AT_SYMLINK_NOFOLLOW;
#ifdef AT_EMPTY_PATH
    allowed |= AT_EMPTY_PATH;
#endif
    if ((unsigned)flags & ~allowed)
    {
        errno = EINVAL;
        return -1;
    }
    unsigned native_flags = flags & AT_SYMLINK_NOFOLLOW ? LUME_FILES_STAT_NOFOLLOW : 0;
#ifdef AT_EMPTY_PATH
    if (flags & AT_EMPTY_PATH)
        native_flags |= LUME_FILES_STAT_EMPTY_PATH;
#endif
    return lume_files_result(lume_native_stat64_at(
        directory == AT_FDCWD ? LUME_FILES_AT_CWD : directory, path, native_flags, metadata));
}
#endif

#endif
