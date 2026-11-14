/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_WAYLAND_RUNTIME_H
#define LUME_WAYLAND_RUNTIME_H
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

static int lume_wayland_runtime(void)
{
    const char *directory = getenv("XDG_RUNTIME_DIR");
    char path[80];
    if (!directory || !*directory)
    {
        snprintf(path, sizeof(path), "/tmp/lume-wayland-%lu", (unsigned long)getuid());
        if (mkdir(path, 0700) && errno != EEXIST)
            return -1;
        directory = path;
    }
    struct stat status;
    if (lstat(directory, &status))
        return -1;
    if (!S_ISDIR(status.st_mode) || status.st_uid != getuid() || (status.st_mode & 0777) != 0700)
    {
        errno = EACCES;
        return -1;
    }
    return setenv("XDG_RUNTIME_DIR", directory, 1);
}
#endif
