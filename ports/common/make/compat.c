#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pwd.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

int fcntl(int fd, int operation, ...)
{
    struct stat metadata;
    if (fstat(fd, &metadata))
        return -1;
    if (operation == F_GETFD)
        return 0;
    if (operation == F_SETFD)
    {
        va_list arguments;
        va_start(arguments, operation);
        int flags = va_arg(arguments, int);
        va_end(arguments);
        if (!flags)
            return 0;
    }
    errno = ENOTSUP;
    return -1;
}

size_t confstr(int name, char *buffer, size_t capacity)
{
    if (name != 0)
    {
        errno = EINVAL;
        return 0;
    }
    static const char path[] = "/data/bin:/bin:/usr/bin";
    if (capacity)
    {
        size_t bytes = capacity < sizeof(path) ? capacity : sizeof(path);
        memcpy(buffer, path, bytes);
        buffer[bytes - 1] = 0;
    }
    return sizeof(path);
}

mode_t umask(mode_t mask)
{
    (void)mask;
    errno = ENOTSUP;
    return (mode_t)-1;
}

int kill(pid_t process, int signal_number)
{
    (void)process;
    (void)signal_number;
    errno = ENOSYS;
    return -1;
}

int pipe(int descriptors[2])
{
    (void)descriptors;
    errno = ENOSYS;
    return -1;
}

int execvp(const char *file, char *const arguments[])
{
    (void)file;
    (void)arguments;
    errno = ENOSYS;
    return -1;
}

int sigprocmask(int operation, const sigset_t *set, sigset_t *previous)
{
    (void)operation;
    (void)set;
    (void)previous;
    errno = ENOSYS;
    return -1;
}

struct passwd *getpwnam(const char *name)
{
    (void)name;
    errno = ENOSYS;
    return NULL;
}

char *getlogin(void)
{
    errno = ENOSYS;
    return NULL;
}
