#include "api.h"
#include "errno_map.h"
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

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
    int result = lume_syscall(LUME_READDIR, (uint32_t)(uintptr_t)directory->path,
                              (uint32_t)(uintptr_t)&entry, directory->index);
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
