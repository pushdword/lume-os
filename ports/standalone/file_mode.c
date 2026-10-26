#include "native.h"
#include <stddef.h>
#include <sys/stat.h>
#include <lume/file_mode.h>

static int change_mode(const char *path, int fd, unsigned mode, uid_t uid, gid_t gid)
{
    struct lume_file_mode_request request = {.version = LUME_FILE_MODE_VERSION,
                                             .bytes = sizeof(request),
                                             .path = (uintptr_t)path,
                                             .fd = fd,
                                             .mode = mode,
                                             .uid = uid,
                                             .gid = gid};
    return native_result(LUME_SYS_FILE_MODE, path ? LUME_FILE_MODE_PATH : LUME_FILE_MODE_FD,
                         (uintptr_t)&request, sizeof(request));
}

int chmod(const char *path, mode_t mode)
{
    return change_mode(path, -1, mode, 0, 0);
}

int fchmod(int fd, mode_t mode)
{
    return change_mode(NULL, fd, mode, 0, 0);
}

static int change_owner(const char *path, int fd, uid_t uid, gid_t gid)
{
    struct stat metadata;
    if ((uid == (uid_t)-1 || gid == (gid_t)-1) &&
        (path ? stat(path, &metadata) : fstat(fd, &metadata)))
        return -1;
    return change_mode(path, fd, UINT32_MAX, uid == (uid_t)-1 ? metadata.st_uid : uid,
                       gid == (gid_t)-1 ? metadata.st_gid : gid);
}

int chown(const char *path, uid_t uid, gid_t gid)
{
    return change_owner(path, -1, uid, gid);
}

int fchown(int fd, uid_t uid, gid_t gid)
{
    return change_owner(NULL, fd, uid, gid);
}
