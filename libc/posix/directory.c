#include "api.h"
#include "errno_map.h"
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#ifdef LUME_POSIX_DIRECTORY_NATIVE
#define lume_stat lume_native_stat
#define LUME_SEEK_SET LUME_NATIVE_SEEK_SET
#define LUME_SEEK_CUR LUME_NATIVE_SEEK_CUR
#define LUME_SEEK_END LUME_NATIVE_SEEK_END
#include "lume/abi.h"
#undef lume_stat
#undef LUME_SEEK_SET
#undef LUME_SEEK_CUR
#undef LUME_SEEK_END
#undef LUME_ARG_BYTES
#include "lume/file_mode.h"
#include "fd.h"
#include "files_client.h"
#ifndef LUME_USER_HOST_TEST
#include "lume/syscall_native.h"
#endif

static int64_t directory_call(unsigned number, uint64_t a, uint64_t b, uint64_t c)
{
#ifdef LUME_USER_HOST_TEST
    extern int64_t lume_abi_host_call(unsigned, uint64_t, uint64_t, uint64_t);
    return lume_abi_host_call(number, a, b, c);
#else
    return lume_native_call(number, a, b, c);
#endif
}

static DIR *directory_fail(DIR *directory, int error)
{
    if (directory->fd >= 0)
        (void)directory_call(LUME_SYS_CLOSE, (unsigned)directory->fd, 0, 0);
    free(directory);
    errno = error;
    return NULL;
}

DIR *opendir(const char *path)
{
    DIR *directory = calloc(1, sizeof(*directory));
    if (!directory)
        return NULL;
    directory->fd = -1;
    struct lume_files_request request = {.path = (uintptr_t)path,
                                         .fd = LUME_FILES_AT_CWD,
                                         .flags = LUME_O_RDONLY | LUME_FILES_OPEN_DIRECTORY,
                                         .descriptor_flags = LUME_FD_CLOEXEC};
    int64_t result = lume_native_files_call(LUME_FILES_OPEN, &request);
    if (result < 0)
        return directory_fail(directory, lume_errno_from_syscall((unsigned)-result));
    if (result > INT_MAX)
    {
        (void)directory_call(LUME_SYS_CLOSE, (uint64_t)result, 0, 0);
        return directory_fail(directory, EOVERFLOW);
    }
    directory->fd = (int)result;
    struct lume_native_stat metadata = {0};
    result = directory_call(LUME_SYS_STAT, (unsigned)directory->fd, (uintptr_t)&metadata, 0);
    if (result < 0)
        return directory_fail(directory, lume_errno_from_syscall((unsigned)-result));
    if (metadata.kind != LUME_NODE_DIR)
        return directory_fail(directory, ENOTDIR);
    return directory;
}

DIR *fdopendir(int fd)
{
    if (fd < 0)
    {
        errno = EBADF;
        return NULL;
    }
    struct lume_native_stat metadata = {0};
    int64_t result = directory_call(LUME_SYS_STAT, (unsigned)fd, (uintptr_t)&metadata, 0);
    if (result < 0 || metadata.kind != LUME_NODE_DIR)
    {
        errno = result < 0 ? lume_errno_from_syscall((unsigned)-result) : ENOTDIR;
        return NULL;
    }
    struct lume_fd_request request = {
        .version = LUME_FD_VERSION, .bytes = sizeof(request), .fd = fd};
    result = directory_call(LUME_SYS_FD, LUME_FD_GETMODE, (uintptr_t)&request, sizeof(request));
    if (result < 0 || (result & 3) == LUME_O_WRONLY)
    {
        errno = result < 0 ? lume_errno_from_syscall((unsigned)-result) : EBADF;
        return NULL;
    }
    result = directory_call(LUME_SYS_SEEK, (unsigned)fd, 0, LUME_NATIVE_SEEK_CUR);
    if (result < 0)
    {
        errno = lume_errno_from_syscall((unsigned)-result);
        return NULL;
    }
    DIR *directory = calloc(1, sizeof(*directory));
    if (directory)
    {
        directory->fd = fd;
        directory->position = (uint64_t)result;
    }
    return directory;
}

int dirfd(DIR *directory)
{
    if (!directory)
    {
        errno = EBADF;
        return -1;
    }
    return directory->fd;
}

struct dirent *readdir(DIR *directory)
{
    if (!directory)
    {
        errno = EBADF;
        return NULL;
    }
    if (directory->next == directory->count)
    {
        if (directory->eof)
            return NULL;
        struct lume_files_request request = {.fd = directory->fd,
                                             .buffer = (uintptr_t)directory->buffer,
                                             .count = sizeof(directory->buffer)};
        int64_t result = lume_native_files_call(LUME_FILES_GETDENTS, &request);
        if (result < 0)
        {
            errno = lume_errno_from_syscall((unsigned)-result);
            return NULL;
        }
        if ((uint64_t)result > sizeof(directory->buffer) ||
            result % (int64_t)sizeof(struct lume_files_dirent))
        {
            errno = EIO;
            return NULL;
        }
        directory->count = (unsigned)((uint64_t)result / sizeof(struct lume_files_dirent));
        directory->next = 0;
        if (!result)
        {
            directory->eof = 1;
            return NULL;
        }
    }
    struct lume_files_dirent entry;
    memcpy(&entry, directory->buffer + directory->next * sizeof(entry), sizeof(entry));
    if (!entry.name_bytes || entry.name_bytes >= sizeof(entry.name) ||
        entry.name[entry.name_bytes] || memchr(entry.name, 0, entry.name_bytes) ||
        entry.inode <= directory->position || (uint64_t)(ino_t)entry.inode != entry.inode)
    {
        errno = EIO;
        return NULL;
    }
    unsigned type = entry.kind == LUME_NODE_FILE     ? DT_REG
                    : entry.kind == LUME_NODE_DIR    ? DT_DIR
                    : entry.kind == LUME_NODE_CHAR   ? DT_CHR
                    : entry.kind == LUME_NODE_PIPE   ? DT_FIFO
                    : entry.kind == LUME_NODE_BLOCK  ? DT_BLK
                    : entry.kind == LUME_NODE_SOCKET ? DT_SOCK
                                                     : DT_UNKNOWN;
    directory->entry = (struct dirent){.d_ino = (ino_t)entry.inode,
                                       .d_reclen = sizeof(struct dirent),
                                       .d_type = (unsigned char)type};
    memcpy(directory->entry.d_name, entry.name, entry.name_bytes + 1);
    ++directory->next;
    /* The VFS inode is also the cursor immediately following this entry. */
    directory->position = entry.inode;
    return &directory->entry;
}

int closedir(DIR *directory)
{
    if (!directory)
    {
        errno = EBADF;
        return -1;
    }
    int64_t result = directory_call(LUME_SYS_CLOSE, (unsigned)directory->fd, 0, 0);
    free(directory);
    if (result < 0)
    {
        errno = lume_errno_from_syscall((unsigned)-result);
        return -1;
    }
    return 0;
}

long telldir(DIR *directory)
{
    if (!directory)
    {
        errno = EBADF;
        return -1;
    }
    if (directory->position > LONG_MAX)
    {
        errno = EOVERFLOW;
        return -1;
    }
    return (long)directory->position;
}

void seekdir(DIR *directory, long position)
{
    if (!directory)
    {
        errno = EBADF;
        return;
    }
    if (position < 0)
    {
        errno = EINVAL;
        return;
    }
    int64_t result = directory_call(LUME_SYS_SEEK, (unsigned)directory->fd, (uint64_t)position, 0);
    if (result < 0)
    {
        errno = lume_errno_from_syscall((unsigned)-result);
        return;
    }
    directory->position = (uint64_t)position;
    directory->count = directory->next = directory->eof = 0;
}

void rewinddir(DIR *directory)
{
    seekdir(directory, 0);
}

#else
DIR *fdopendir(int fd)
{
    (void)fd;
    errno = ENOTSUP;
    return NULL;
}

DIR *opendir(const char *path)
{
    struct stat metadata;
    if (stat(path, &metadata))
        return NULL;
    if (!S_ISDIR(metadata.st_mode))
    {
        errno = ENOTDIR;
        return NULL;
    }
    DIR *directory = calloc(1, sizeof(*directory));
    if (!directory)
        return NULL;
    size_t length = 0;
    if (*path != '/')
    {
        if (!getcwd(directory->path, sizeof(directory->path)))
            goto fail;
        length = strlen(directory->path);
        if (length + 1 >= sizeof(directory->path))
            goto too_long;
        directory->path[length++] = '/';
    }
    size_t bytes = strlen(path);
    if (bytes >= sizeof(directory->path) - length)
        goto too_long;
    memcpy(directory->path + length, path, bytes + 1);
    return directory;
too_long:
    errno = ENAMETOOLONG;
fail:
    free(directory);
    return NULL;
}

struct dirent *readdir(DIR *directory)
{
    if (!directory)
    {
        errno = EBADF;
        return NULL;
    }
    struct lume_directory_entry entry;
    int result = lume_app_call(LUME_READDIR, (uintptr_t)directory->path, (uintptr_t)&entry,
                               directory->index);
    if (result <= 0)
    {
        if (result < 0)
            errno = lume_errno_from_syscall((unsigned)-result);
        return NULL;
    }
    directory->index = (unsigned)result;
    directory->entry = (struct dirent){.d_ino = (ino_t)result,
                                       .d_reclen = sizeof(struct dirent),
                                       .d_type = (entry.mode & 0170000) == 0040000   ? DT_DIR
                                                 : (entry.mode & 0170000) == 0060000 ? DT_BLK
                                                 : (entry.mode & 0170000) == 0020000 ? DT_CHR
                                                 : (entry.mode & 0170000) == 0010000 ? DT_FIFO
                                                                                     : DT_REG};
    memcpy(directory->entry.d_name, entry.name, sizeof(entry.name));
    directory->entry.d_name[sizeof(directory->entry.d_name) - 1] = 0;
    return &directory->entry;
}

int closedir(DIR *directory)
{
    if (!directory)
    {
        errno = EBADF;
        return -1;
    }
    free(directory);
    return 0;
}

void rewinddir(DIR *directory)
{
    directory->index = 0;
}

long telldir(DIR *directory)
{
    return (long)directory->index;
}

void seekdir(DIR *directory, long position)
{
    if (position < 0 || (unsigned long)position > UINT_MAX)
    {
        errno = EINVAL;
        return;
    }
    directory->index = (unsigned)position;
}

#endif
