#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

char *realpath(const char *path, char *resolved)
{
    char result[128];
    size_t used;
    if (!path || !*path)
    {
        errno = ENOENT;
        return NULL;
    }
    if (*path == '/')
        strcpy(result, "/");
    else if (!getcwd(result, sizeof(result)))
        return NULL;
    used = strlen(result);
    while (*path)
    {
        while (*path == '/')
            ++path;
        if (!*path)
            break;
        const char *end = strchr(path, '/');
        size_t count = end ? (size_t)(end - path) : strlen(path);
        if (count == 1 && *path == '.')
        {
            path += count;
            continue;
        }
        if (count == 2 && path[0] == '.' && path[1] == '.')
        {
            while (used > 1 && result[used - 1] != '/')
                --used;
            if (used > 1)
                --used;
            result[used] = 0;
            path += count;
            continue;
        }
        size_t separator = used > 1;
        if (count >= sizeof(result) - used - separator)
        {
            errno = ENAMETOOLONG;
            return NULL;
        }
        if (separator)
            result[used++] = '/';
        memcpy(result + used, path, count);
        used += count;
        result[used] = 0;
        struct stat metadata;
        if (lstat(result, &metadata))
            return NULL;
        if (S_ISLNK(metadata.st_mode))
        {
            errno = ENOTSUP;
            return NULL;
        }
        if (end && !S_ISDIR(metadata.st_mode))
        {
            errno = ENOTDIR;
            return NULL;
        }
        path += count;
    }
    if (!resolved)
        resolved = malloc(used + 1);
    if (!resolved)
    {
        errno = ENOMEM;
        return NULL;
    }
    memcpy(resolved, result, used + 1);
    return resolved;
}
