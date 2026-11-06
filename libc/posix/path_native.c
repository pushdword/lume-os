/* SPDX-License-Identifier: BSD-3-Clause */
#include "native_posix.h"
#include <fd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ftw.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include "files_client.h"

int openat(int directory, const char *path, int flags, ...)
{
    if (!path)
    {
        errno = EFAULT;
        return -1;
    }
    mode_t mode = 0;
    if (flags & O_CREAT)
    {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, unsigned);
        va_end(args);
    }
    if (*path == '/' || directory == AT_FDCWD)
        return open(path, flags, mode);
    unsigned known = O_ACCMODE | O_CREAT | O_TRUNC | O_APPEND | O_EXCL | O_CLOEXEC | O_NONBLOCK |
                     O_NOCTTY | O_NOFOLLOW | O_DIRECTORY;
    if (((unsigned)flags & ~known) || (flags & O_ACCMODE) > O_RDWR)
    {
        errno = EINVAL;
        return -1;
    }
    struct lume_files_request request = {.path = (uintptr_t)path,
                                         .fd = directory,
                                         .flags = (flags & O_ACCMODE) == O_WRONLY ? LUME_O_WRONLY
                                                  : (flags & O_ACCMODE) == O_RDWR ? LUME_O_RDWR
                                                                                  : LUME_O_RDONLY,
                                         .mode = mode & 0777u,
                                         .descriptor_flags =
                                             (flags & O_CLOEXEC ? LUME_FD_CLOEXEC : 0) |
                                             (flags & O_NONBLOCK ? LUME_FD_NONBLOCK : 0)};
    if (flags & O_CREAT)
        request.flags |= LUME_O_CREAT;
    if (flags & O_TRUNC)
        request.flags |= LUME_O_TRUNC;
    if (flags & O_APPEND)
        request.flags |= LUME_O_APPEND;
    if (flags & O_EXCL)
        request.flags |= LUME_O_EXCL;
    if (flags & O_DIRECTORY)
        request.flags |= LUME_FILES_OPEN_DIRECTORY;
    if (flags & O_NOFOLLOW)
        request.flags |= LUME_FILES_OPEN_NOFOLLOW;
    int result = lume_files_result(lume_native_files_call(LUME_FILES_OPEN, &request));
    if (result >= 0 && !(flags & O_NOCTTY))
    {
        extern void lume_posix_acquire_terminal(int);
        lume_posix_acquire_terminal(result);
    }
    return result;
}

int fstatat(int directory, const char *path, struct stat *metadata, int flags)
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
    return lume_files_result(lume_native_stat_at(
        directory == AT_FDCWD ? LUME_FILES_AT_CWD : directory, path, native_flags, metadata));
}

int faccessat(int directory, const char *path, int mode, int flags)
{
    if (mode & ~(R_OK | W_OK | X_OK))
    {
        errno = EINVAL;
        return -1;
    }
    if (flags & ~AT_SYMLINK_NOFOLLOW)
    {
        errno = ENOTSUP;
        return -1;
    }
    struct stat metadata;
    if (fstatat(directory, path, &metadata, flags))
        return -1;
    unsigned permissions = metadata.st_mode;
    if (((mode & R_OK) && !(permissions & 0444)) || ((mode & W_OK) && !(permissions & 0222)) ||
        ((mode & X_OK) && !(permissions & 0111)))
    {
        errno = EACCES;
        return -1;
    }
    return 0;
}

ssize_t readlink(const char *path, char *buffer, size_t bytes)
{
    (void)buffer;
    (void)bytes;
    struct stat metadata;
    if (lstat(path, &metadata))
        return -1;
    errno = EINVAL;
    return -1;
}

char *mkdtemp(char *pattern)
{
    size_t length = strlen(pattern);
    if (length < 6 || strcmp(pattern + length - 6, "XXXXXX"))
    {
        errno = EINVAL;
        return NULL;
    }
    for (unsigned attempt = 0; attempt < 64; ++attempt)
    {
        unsigned char random[6];
        if (native_result(native_call(LUME_SYS_RANDOM, (uintptr_t)random, sizeof(random), 0)) < 0)
            return NULL;
        for (unsigned i = 0; i < 6; ++i)
            pattern[length - 6 + i] = "abcdefghijklmnopqrstuvwxyz0123456789"[random[i] % 36];
        if (!mkdir(pattern, 0700))
            return pattern;
        if (errno != EEXIST)
            return NULL;
    }
    errno = EEXIST;
    return NULL;
}

static int tree_walk(const char *path,
                     int (*visit)(const char *, const struct stat *, int, struct FTW *), int flags,
                     int level)
{
    struct stat metadata;
    struct FTW info = {.level = level};
    const char *slash = strrchr(path, '/');
    info.base = slash ? (int)(slash - path + 1) : 0;
    if (lstat(path, &metadata))
        return visit(path, NULL, FTW_NS, &info);
    if (!S_ISDIR(metadata.st_mode))
        return visit(path, &metadata, FTW_F, &info);
    if (level >= 32)
    {
        errno = ELOOP;
        return -1;
    }
    if (!(flags & FTW_DEPTH))
    {
        int result = visit(path, &metadata, FTW_D, &info);
        if (result)
            return result;
    }
    DIR *directory = opendir(path);
    if (!directory)
        return visit(path, &metadata, FTW_DNR, &info);
    struct dirent *entry;
    int result = 0;
    while ((entry = readdir(directory)))
    {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;
        char child[PATH_MAX];
        if (snprintf(child, sizeof(child), "%s/%s", path, entry->d_name) >= (int)sizeof(child))
        {
            errno = ENAMETOOLONG;
            result = -1;
            break;
        }
        if ((result = tree_walk(child, visit, flags, level + 1)))
            break;
    }
    closedir(directory);
    return result ? result : flags & FTW_DEPTH ? visit(path, &metadata, FTW_DP, &info) : 0;
}

int nftw(const char *path, int (*visit)(const char *, const struct stat *, int, struct FTW *),
         int descriptors, int flags)
{
    if (descriptors < 1 || flags & ~(FTW_PHYS | FTW_DEPTH))
    {
        errno = ENOTSUP;
        return -1;
    }
    return tree_walk(path, visit, flags, 0);
}

char *realpath(const char *path, char *resolved)
{
    if (!path || !*path)
    {
        errno = ENOENT;
        return NULL;
    }
    char *output = resolved ? resolved : malloc(PATH_MAX);
    if (!output)
    {
        errno = ENOMEM;
        return NULL;
    }
    struct lume_files_request request = {.path = (uintptr_t)path,
                                         .fd = LUME_FILES_AT_CWD,
                                         .buffer = (uintptr_t)output,
                                         .count = PATH_MAX};
    if (lume_files_result(lume_native_files_call(LUME_FILES_CANONICAL, &request)) < 0)
    {
        if (!resolved)
            free(output);
        return NULL;
    }
    return output;
}
