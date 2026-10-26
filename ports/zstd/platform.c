#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <lume/abi.h>
#include <lume/file_mode.h>
#include <lume/syscall_native.h>
#include <errno_map.h>

clock_t lume_zstd_progress_clock(void)
{
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now))
        return (clock_t)-1;
    return (clock_t)((uint64_t)now.tv_sec * CLOCKS_PER_SEC +
                     (uint64_t)now.tv_nsec * CLOCKS_PER_SEC / 1000000000u);
}

static int change_mode(const char *path, unsigned mode, uid_t uid, gid_t gid)
{
    struct lume_file_mode_request request = {.version = LUME_FILE_MODE_VERSION,
                                             .bytes = sizeof(request),
                                             .path = (uintptr_t)path,
                                             .fd = -1,
                                             .mode = mode,
                                             .uid = uid,
                                             .gid = gid};
    int64_t result = lume_native_call(LUME_SYS_FILE_MODE, LUME_FILE_MODE_PATH, (uintptr_t)&request,
                                      sizeof(request));
    if (result < 0)
    {
        errno = lume_errno_from_syscall((unsigned)-result);
        return -1;
    }
    return 0;
}

int chmod(const char *path, mode_t mode)
{
    return change_mode(path, mode, 0, 0);
}

int chown(const char *path, uid_t uid, gid_t gid)
{
    struct stat metadata;
    if ((uid == (uid_t)-1 || gid == (gid_t)-1) && stat(path, &metadata))
        return -1;
    return change_mode(path, UINT32_MAX, uid == (uid_t)-1 ? metadata.st_uid : uid,
                       gid == (gid_t)-1 ? metadata.st_gid : gid);
}
