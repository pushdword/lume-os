/* SPDX-License-Identifier: BSD-3-Clause */
#include "stdio_support.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int lume_coreutils_freopen_flags(const char *mode)
{
    bool update = strchr(mode, '+') != NULL;
    int flags;

    switch (mode[0])
    {
        case 'r':
            flags = update ? O_RDWR : O_RDONLY;
            break;
        case 'w':
            flags = (update ? O_RDWR : O_WRONLY) | O_CREAT | O_TRUNC;
            break;
        case 'a':
            flags = (update ? O_RDWR : O_WRONLY) | O_CREAT | O_APPEND;
            break;
        default:
            errno = EINVAL;
            return -1;
    }

    if (strchr(mode, 'x'))
    {
        flags |= O_EXCL;
    }
#ifdef O_CLOEXEC
    if (strchr(mode, 'e'))
    {
        flags |= O_CLOEXEC;
    }
#endif
    return flags;
}

FILE *lume_coreutils_freopen(const char *path, const char *mode, FILE *stream)
{
    if (!path || !mode || !stream)
    {
        errno = EINVAL;
        return NULL;
    }

    int flags = lume_coreutils_freopen_flags(mode);
    if (flags < 0)
    {
        return NULL;
    }

    int target = fileno(stream);
    if (target < 0)
    {
        return NULL;
    }

    int replacement = open(path, flags, 0666);
    if (replacement < 0)
    {
        return NULL;
    }

    if (lume_coreutils_write_pending(stream) && fflush(stream) != 0)
    {
        int saved_errno = errno;
        close(replacement);
        errno = saved_errno;
        return NULL;
    }
    lume_coreutils_purge(stream);
    clearerr(stream);

    if (replacement != target)
    {
        if (dup2(replacement, target) < 0)
        {
            int saved_errno = errno;
            close(replacement);
            errno = saved_errno;
            return NULL;
        }
        close(replacement);
    }
    return stream;
}
