#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pwd.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

int __real_fileno(FILE *stream);

int __wrap_fileno(FILE *stream)
{
    if (stream == stdin)
        return 0;
    if (stream == stdout)
        return 1;
    if (stream == stderr)
        return 2;
    return __real_fileno(stream);
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
