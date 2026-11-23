/* SPDX-License-Identifier: BSD-3-Clause */
#include "auth_files.h"
#include "lume/abi.h"
#include "lume/files.h"
#include "lume/syscall_native.h"
#include <errno.h>
#include <fcntl.h>
#include <fd.h>
#include <stdint.h>
#include <sys/random.h>
#include <unistd.h>

bool auth_permissions_volume(int directory, bool writable)
{
    struct lume_files_volume volume;
    struct lume_files_request request = {.version = LUME_FILES_VERSION,
                                         .bytes = sizeof(request),
                                         .buffer = (uintptr_t)&volume,
                                         .count = sizeof(volume),
                                         .fd = directory,
                                         .flags = LUME_FILES_VOLUME_FD |
                                                  LUME_FILES_VOLUME_QUERY_PERMISSIONS};
    if (lume_native_call(LUME_SYS_FILES, LUME_FILES_VOLUME, (uintptr_t)&request, sizeof(request)) <
            0 ||
        !(volume.flags & LUME_FILES_VOLUME_NATIVE_PERMISSIONS) ||
        (writable && (volume.flags & LUME_FILES_VOLUME_READ_ONLY)))
    {
        errno = ENOTSUP;
        return false;
    }
    return true;
}

int auth_trusted_directory(int parent, const char *name, bool create, mode_t mode)
{
    int flags = O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC;
    int fd = openat(parent, name, flags);
    if (fd < 0 && errno == ENOENT && create)
    {
        if (mkdirat(parent, name, mode) < 0 && errno != EEXIST)
            return -1;
        fd = openat(parent, name, flags);
    }
    if (fd < 0)
        return -1;
    struct stat info;
    if (!fstat(fd, &info) && S_ISDIR(info.st_mode) && !info.st_uid &&
        !(info.st_mode & (mode == 0700 ? 077u : 022u)))
        return fd;
    close(fd);
    errno = EACCES;
    return -1;
}

bool auth_close_descriptors(void)
{
    struct lume_fd_request request = {
        .version = LUME_FD_VERSION, .bytes = sizeof(request), .fd = 3};
    return lume_native_call(LUME_SYS_FD, LUME_FD_CLOSE_FROM, (uintptr_t)&request,
                            sizeof(request)) >= 0;
}

bool auth_random(unsigned char *buffer, size_t bytes)
{
    size_t offset = 0;
    while (offset < bytes)
    {
        ssize_t got = getrandom(buffer + offset, bytes - offset, 0);
        if (got > 0)
            offset += (size_t)got;
        else if (!got || errno != EINTR)
            return false;
    }
    return true;
}
