#include <errno.h>
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
