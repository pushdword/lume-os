#include "native.h"
#include <stddef.h>
#include <sys/stat.h>
#include <lume/file_mode.h>
#include "file_owner_client.h"

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
    struct lume_files_request request = {
        .path = (uintptr_t)path, .fd = LUME_FILES_AT_CWD, .mode = mode & 07777u};
    return lume_files_result(lume_native_files_call(LUME_FILES_MODE, &request));
}

int fchmod(int fd, mode_t mode)
{
    return change_mode(NULL, fd, mode & ~(mode_t)S_IFMT, 0, 0);
}

int chown(const char *path, uid_t uid, gid_t gid)
{
    return lume_files_change_owner(LUME_FILE_MODE_OWNER, path, -1, uid, gid);
}

int fchown(int fd, uid_t uid, gid_t gid)
{
    return lume_files_change_owner(LUME_FILE_MODE_OWNER_FD, NULL, fd, uid, gid);
}
