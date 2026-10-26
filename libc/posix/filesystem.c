#include "api.h"
#include "errno_map.h"
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

int lstat(const char *path, struct stat *metadata)
{
    return stat(path, metadata);
}

int access(const char *path, int mode)
{
    if (mode & ~(R_OK | W_OK | X_OK))
    {
        errno = EINVAL;
        return -1;
    }
    struct stat metadata;
    if (stat(path, &metadata))
        return -1;
    unsigned permissions = metadata.st_mode;
    if (((mode & R_OK) && !(permissions & 0444)) || ((mode & W_OK) && !(permissions & 0222)) ||
        ((mode & X_OK) && !(permissions & 0111)))
    {
        errno = EACCES;
        return -1;
    }
    return 0;
}

int ftruncate(int fd, off_t length)
{
    struct stat metadata;
    if (length < 0)
    {
        errno = EINVAL;
        return -1;
    }
    if (fstat(fd, &metadata))
        return -1;
    if (!S_ISREG(metadata.st_mode))
    {
        errno = EINVAL;
        return -1;
    }
    if (length > metadata.st_size)
    {
        errno = ENOTSUP;
        return -1;
    }
    off_t previous = lseek(fd, 0, SEEK_CUR);
    if (previous < 0 || lseek(fd, length, SEEK_SET) < 0)
        return -1;
    int result = lume_syscall(LUME_TRUNCATE_HERE, fd, 0, 0);
    int saved = result < 0 ? lume_errno_from_syscall((unsigned)-result) : 0;
    if (lseek(fd, previous, SEEK_SET) < 0 && !saved)
        return -1;
    if (saved)
    {
        errno = saved;
        return -1;
    }
    return 0;
}
