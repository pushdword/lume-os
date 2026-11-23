#ifndef LUME_FILE_OWNER_CLIENT_H
#define LUME_FILE_OWNER_CLIENT_H
#include "files_client.h"
#include "lume/file_mode.h"
#include "lume/syscall_native.h"
#include "lume/syscall_numbers.h"

static inline int lume_files_change_owner(unsigned operation, const char *path, int fd,
                                          uid_t owner, gid_t group)
{
    struct lume_file_mode_request request = {
        .version = LUME_FILE_MODE_VERSION,
        .bytes = sizeof(request),
        .path = (uintptr_t)path,
        .fd = fd,
        .mode = UINT32_MAX,
        .uid = owner == (uid_t)-1 ? UINT32_MAX : (uint32_t)owner,
        .gid = group == (gid_t)-1 ? UINT32_MAX : (uint32_t)group};
    return lume_files_result(lume_native_call(LUME_SYS_FILE_MODE, operation, (uintptr_t)&request,
                                              sizeof(request)));
}
#endif
