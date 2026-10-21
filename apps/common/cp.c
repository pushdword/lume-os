#include "api.h"
#include <string.h>

static int canonical(const char *path, char output[128])
{
    unsigned used = 1;
    output[0] = '/';
    output[1] = 0;
    if (*path != '/')
    {
        if (lume_syscall(LUME_GETCWD, (uint32_t)(uintptr_t)output, 128, 0) < 0)
            return -1;
        used = (unsigned)strlen(output);
    }
    while (*path)
    {
        while (*path == '/')
            ++path;
        const char *part = path;
        while (*path && *path != '/')
            ++path;
        unsigned length = (unsigned)(path - part);
        if (!length || (length == 1 && part[0] == '.'))
            continue;
        if (length == 2 && part[0] == '.' && part[1] == '.')
        {
            while (used > 1 && output[used - 1] != '/')
                --used;
            if (used > 1)
                --used;
            output[used] = 0;
            continue;
        }
        if (used + length + 1 >= 128)
            return -1;
        if (used > 1)
            output[used++] = '/';
        memcpy(output + used, part, length);
        used += length;
        output[used] = 0;
    }
    return 0;
}

int app_main(int argc, char **argv)
{
    char source[128], target[128];
    if (argc != 3)
    {
        app_puts("usage: cp SOURCE DESTINATION\n");
        return 2;
    }
    if (canonical(argv[1], source) || canonical(argv[2], target) || !strcmp(source, target))
    {
        app_puts("cp: invalid or identical paths\n");
        return 1;
    }
    /* FAT path matching ignores ASCII case. */
    unsigned same = 1;
    for (unsigned i = 0;; ++i)
    {
        unsigned a = (unsigned char)source[i], b = (unsigned char)target[i];
        if (a >= 'A' && a <= 'Z')
            a += 'a' - 'A';
        if (b >= 'A' && b <= 'Z')
            b += 'a' - 'A';
        if (a != b)
        {
            same = 0;
            break;
        }
        if (!a)
            break;
    }
    if (same)
    {
        app_puts("cp: identical paths\n");
        return 1;
    }
    int in = app_open(argv[1], LUME_RDONLY);
    if (in < 0)
    {
        app_error("cp", in);
        return 1;
    }
    int out = app_open(argv[2], LUME_WRONLY | LUME_CREAT | LUME_TRUNC);
    if (out < 0)
    {
        app_close(in);
        app_error("cp", out);
        return 1;
    }
    char buffer[512];
    int n, error = 0;
    while ((n = app_read(in, buffer, sizeof(buffer))) > 0)
    {
        unsigned offset = 0;
        while (offset < (unsigned)n)
        {
            int written = app_write(out, buffer + offset, (unsigned)n - offset);
            if (written <= 0)
            {
                error = written ? written : -5;
                break;
            }
            offset += (unsigned)written;
        }
        if (error)
            break;
    }
    if (n < 0)
        error = n;
    int closed = app_close(out);
    (void)app_close(in);
    if (!error && closed < 0)
        error = closed;
    if (error)
        app_error("cp", error);
    return error != 0;
}
