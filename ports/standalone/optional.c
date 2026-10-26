#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

int ioctl(int fd, unsigned long request, ...)
{
    (void)fd;
    (void)request;
    errno = ENOTTY;
    return -1;
}

int symlink(const char *target, const char *path)
{
    (void)target;
    (void)path;
    errno = ENOTSUP;
    return -1;
}

ssize_t readlink(const char *path, char *buffer, size_t bytes)
{
    (void)path;
    (void)buffer;
    (void)bytes;
    errno = ENOTSUP;
    return -1;
}

int utimes(const char *path, const struct timeval times[2])
{
    (void)path;
    (void)times;
    errno = ENOTSUP;
    return -1;
}

int gethostname(char *name, size_t bytes)
{
    if (bytes < sizeof("lume"))
    {
        errno = ENAMETOOLONG;
        return -1;
    }
    memcpy(name, "lume", sizeof("lume"));
    return 0;
}
