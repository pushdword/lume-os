/* SPDX-License-Identifier: BSD-3-Clause */
#define LUME_STAT_IMPLEMENTATION 1
#define _LARGEFILE64_SOURCE 1
#define lume_stat lume_native_stat
#include "lume/abi.h"
#undef lume_stat
#include "files_client.h"
#include "../../libc/runtime/stat64.h"
#include <fcntl.h>
#include <string.h>
static uint64_t inode, bytes;
static int status, calls;
static struct lume_files_request observed;

int64_t lume_abi_host_call(unsigned number, uint64_t action, uint64_t pointer, uint64_t count)
{
    if (number != LUME_SYS_FILES || action != LUME_FILES_STAT || count != sizeof(observed))
        return -LUME_EINVAL;
    struct lume_files_request *request = (void *)(uintptr_t)pointer;
    if (request->version != LUME_FILES_VERSION || request->bytes != sizeof(*request))
        return -LUME_EINVAL;
    observed = *request;
    ++calls;
    if (status < 0)
        return status;
    *(struct lume_native_stat *)(uintptr_t)request->buffer =
        (struct lume_native_stat){inode, bytes, LUME_NODE_CHAR, 0};
    return status;
}

#define CHECK(x)                                                                                   \
    do                                                                                             \
    {                                                                                              \
        if (!(x))                                                                                  \
            return __LINE__;                                                                       \
    } while (0)

int main(void)
{
    struct stat64 value;
    struct stat legacy;
    inode = 0x1234;
    bytes = 100;
    status = 0100644;
    CHECK(!lume_native_stat_at(3, "", LUME_FILES_STAT_EMPTY_PATH, &legacy));
    CHECK(legacy.st_ino == inode && legacy.st_size == bytes && legacy.st_mode == status);
    inode = 0x200003;
    bytes = 0;
    status = 0020600;
    CHECK(!fstat64(3, &value));
    CHECK(value.st_ino == inode && value.st_mode == status && !value.st_size);
    CHECK(observed.fd == 3 && observed.flags == LUME_FILES_STAT_EMPTY_PATH);
    inode = UINT64_C(0xfedcba9876543210);
    bytes = (UINT64_C(1) << 40) + 17;
    status = 0100644;
    CHECK(!stat64("/data/file", &value));
    CHECK(value.st_ino == inode && value.st_size == bytes &&
          value.st_blocks == (bytes + 511) / 512);
    CHECK(observed.fd == LUME_FILES_AT_CWD && !observed.flags);
    CHECK(!lstat64("/data/link", &value));
    CHECK(observed.flags == LUME_FILES_STAT_NOFOLLOW);
    CHECK(!fstatat64(AT_FDCWD, "/data/file", &value, AT_SYMLINK_NOFOLLOW));
    CHECK(observed.fd == LUME_FILES_AT_CWD && observed.flags == LUME_FILES_STAT_NOFOLLOW);
    int before = calls;
    CHECK(fstatat64(3, "x", &value, 0x40000000) == -1 && errno == EINVAL && calls == before);
    CHECK(fstat64(3, 0) == -1 && errno == EFAULT && calls == before);
    bytes = UINT64_MAX;
    CHECK(fstat64(3, &value) == -1 && errno == EOVERFLOW);
    bytes = 0;
    status = -LUME_EBADF;
    CHECK(fstat64(3, &value) == -1 && errno == EBADF);
    return 0;
}
