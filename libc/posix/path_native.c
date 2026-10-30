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

static int relative_path(int fd, const char *path, char out[LUME_PATH_MAX])
{
    if (*path == '/' || fd == AT_FDCWD)
    {
        if (strlen(path) >= LUME_PATH_MAX)
        {
            errno = ENAMETOOLONG;
            return -1;
        }
        strcpy(out, path);
        return 0;
    }
    struct lume_fd_request r = {.version = LUME_FD_VERSION,
                                .bytes = sizeof(r),
                                .fd = fd,
                                .buffer = (uintptr_t)out,
                                .count = LUME_PATH_MAX};
    if (native_result(native_call(LUME_SYS_FD, LUME_FD_PATH, (uintptr_t)&r, sizeof(r))))
        return -1;
    size_t length = strlen(out), count = strlen(path);
    if (length + count + 2 > LUME_PATH_MAX)
    {
        errno = ENAMETOOLONG;
        return -1;
    }
    out[length++] = '/';
    memcpy(out + length, path, count + 1);
    return 0;
}

int openat(int directory, const char *path, int flags, ...)
{
    char name[LUME_PATH_MAX];
    if (relative_path(directory, path, name))
        return -1;
    mode_t mode = 0;
    if (flags & O_CREAT)
    {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, unsigned);
        va_end(args);
    }
    return open(name, flags, mode);
}

int fstatat(int directory, const char *path, struct stat *metadata, int flags)
{
    char name[LUME_PATH_MAX];
    if (flags & ~AT_SYMLINK_NOFOLLOW)
    {
        errno = EINVAL;
        return -1;
    }
    return relative_path(directory, path, name) ? -1 : stat(name, metadata);
}

int faccessat(int directory, const char *path, int mode, int flags)
{
    char name[LUME_PATH_MAX];
    if (flags)
    {
        errno = ENOTSUP;
        return -1;
    }
    return relative_path(directory, path, name) ? -1 : access(name, mode);
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
        char child[LUME_PATH_MAX];
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
    char input[LUME_PATH_MAX], result[LUME_PATH_MAX];
    if (!path || !*path)
    {
        errno = ENOENT;
        return NULL;
    }
    if (*path == '/')
    {
        if (strlen(path) >= sizeof(input))
        {
            errno = ENAMETOOLONG;
            return NULL;
        }
        strcpy(input, path);
    }
    else
    {
        char cwd[LUME_PATH_MAX];
        if (!getcwd(cwd, sizeof(cwd)))
            return NULL;
        if (snprintf(input, sizeof(input), "%s/%s", cwd, path) >= (int)sizeof(input))
        {
            errno = ENAMETOOLONG;
            return NULL;
        }
    }
    strcpy(result, "/");
    char *save, *part = strtok_r(input, "/", &save);
    while (part)
    {
        if (!strcmp(part, ".."))
        {
            char *slash = strrchr(result, '/');
            if (slash == result)
                result[1] = 0;
            else
                *slash = 0;
        }
        else if (strcmp(part, "."))
        {
            if (result[1])
                strcat(result, "/");
            strcat(result, part);
            struct stat metadata;
            if (stat(result, &metadata))
                return NULL;
            if (save && *save && !S_ISDIR(metadata.st_mode))
            {
                errno = ENOTDIR;
                return NULL;
            }
        }
        part = strtok_r(NULL, "/", &save);
    }
    if (!resolved && !(resolved = malloc(strlen(result) + 1)))
    {
        errno = ENOMEM;
        return NULL;
    }
    strcpy(resolved, result);
    return resolved;
}
