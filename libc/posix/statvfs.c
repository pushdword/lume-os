#include <sys/statvfs.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stddef.h>
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
#include "files_client.h"
#endif

static int volume_query(const char *path, int fd, struct statvfs *out)
{
    if (!out)
    {
        errno = EFAULT;
        return -1;
    }
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
    struct lume_files_volume volume = {0};
    struct lume_files_request request = {.path = (uintptr_t)path,
                                         .buffer = (uintptr_t)&volume,
                                         .count = sizeof(volume),
                                         .fd = path ? LUME_FILES_AT_CWD : fd,
                                         .flags = path ? 0 : LUME_FILES_VOLUME_FD};
    if (lume_files_result(lume_native_files_call(LUME_FILES_VOLUME, &request)) < 0)
        return -1;
    if (volume.version != LUME_FILES_VERSION || volume.bytes != sizeof(volume) ||
        !volume.block_bytes || !volume.fragment_bytes || volume.free_blocks > volume.total_blocks ||
        volume.available_blocks > volume.free_blocks ||
        (volume.flags & ~(uint32_t)LUME_FILES_VOLUME_READ_ONLY))
    {
        errno = EIO;
        return -1;
    }
    if (volume.block_bytes > ULONG_MAX || volume.fragment_bytes > ULONG_MAX ||
        volume.filesystem_id > ULONG_MAX)
    {
        errno = EOVERFLOW;
        return -1;
    }
    *out = (struct statvfs){.f_bsize = (unsigned long)volume.block_bytes,
                            .f_frsize = (unsigned long)volume.fragment_bytes,
                            .f_blocks = volume.total_blocks,
                            .f_bfree = volume.free_blocks,
                            .f_bavail = volume.available_blocks,
                            .f_files = (fsfilcnt_t)-1,
                            .f_ffree = (fsfilcnt_t)-1,
                            .f_favail = (fsfilcnt_t)-1,
                            .f_fsid = (unsigned long)volume.filesystem_id,
                            .f_flag = volume.flags & LUME_FILES_VOLUME_READ_ONLY ? ST_RDONLY : 0,
                            .f_namemax = volume.name_max};
    return 0;
#else
    (void)path;
    (void)fd;
    errno = ENOTSUP;
    return -1;
#endif
}

int statvfs(const char *path, struct statvfs *out)
{
    if (!path)
    {
        errno = EFAULT;
        return -1;
    }
    return volume_query(path, -1, out);
}

int fstatvfs(int fd, struct statvfs *out)
{
    return volume_query(NULL, fd, out);
}
