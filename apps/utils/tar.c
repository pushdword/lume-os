#include "api.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define PATH_CAPACITY 128u
static unsigned char buffer[65536];

static int octal(const unsigned char *text, unsigned bytes, uint64_t *value)
{
    *value = 0;
    unsigned i = 0;
    while (i < bytes && text[i] == ' ')
        ++i;
    for (; i < bytes && text[i] >= '0' && text[i] <= '7'; ++i)
    {
        if (*value > (UINT64_MAX - 7) / 8)
            return -1;
        *value = *value * 8 + text[i] - '0';
    }
    for (; i < bytes; ++i)
        if (text[i] && text[i] != ' ')
            return -1;
    return 0;
}

static int append(char *out, size_t *used, const unsigned char *text, unsigned bytes)
{
    for (unsigned i = 0; i < bytes && text[i]; ++i)
    {
        if (*used + 1 >= PATH_CAPACITY)
            return -1;
        out[(*used)++] = (char)text[i];
    }
    out[*used] = 0;
    return 0;
}

static int safe_name(char *out, const unsigned char *header)
{
    size_t used = 0;
    out[0] = 0;
    if (!memcmp(header + 257, "ustar", 5) && header[345])
    {
        if (append(out, &used, header + 345, 155) || used + 1 >= PATH_CAPACITY)
            return -1;
        out[used++] = '/';
    }
    if (append(out, &used, header, 100) || !used || out[0] == '/')
        return -1;
    for (unsigned i = 0; i < used;)
    {
        unsigned first = i;
        while (i < used && out[i] != '/')
            ++i;
        if (i - first == 2 && out[first] == '.' && out[first + 1] == '.')
            return -1;
        ++i;
    }
    return 0;
}

static int directory(const char *path)
{
    struct stat metadata;
    if (stat(path, &metadata) == 0)
        return S_ISDIR(metadata.st_mode) ? 0 : -1;
    return mkdir(path, 0777);
}

static int parents(char *path, int include_last)
{
    for (unsigned i = 1; path[i]; ++i)
        if (path[i] == '/')
        {
            path[i] = 0;
            int result = directory(path);
            path[i] = '/';
            if (result)
                return -1;
        }
    return include_last ? directory(path) : 0;
}

static int read_exact(int fd, void *buffer, size_t bytes)
{
    unsigned char *out = buffer;
    while (bytes)
    {
        ssize_t count = read(fd, out, bytes);
        if (count <= 0 || (size_t)count > bytes)
            return -1;
        bytes -= (size_t)count;
        out += count;
    }
    return 0;
}

static int transfer(int archive, int output, uint64_t bytes)
{
    while (bytes)
    {
        size_t count = bytes > sizeof(buffer) ? sizeof(buffer) : (size_t)bytes;
        if (read_exact(archive, buffer, count))
            return -1;
        if (output >= 0)
            for (size_t offset = 0; offset < count;)
            {
                ssize_t written = write(output, buffer + offset, count - offset);
                if (written <= 0 || (size_t)written > count - offset)
                    return -1;
                offset += (size_t)written;
            }
        bytes -= count;
    }
    return 0;
}

int app_main(unsigned argc, char **argv, char **envp)
{
    (void)envp;
    if ((argc != 3 && argc != 5) || (strcmp(argv[1], "-xf") && strcmp(argv[1], "-tf")) ||
        (argc == 5 && strcmp(argv[3], "-C")))
    {
        fputs("usage: tar -xf|-tf archive [-C directory]\n", stderr);
        return 2;
    }
    int extract = !strcmp(argv[1], "-xf");
    int archive = open(argv[2], O_RDONLY);
    if (archive < 0)
    {
        perror("tar");
        return 1;
    }
    const char *root = argc == 5 ? argv[4] : ".";
    int result = 1;
    unsigned zeros = 0;
    for (;;)
    {
        unsigned char header[512];
        if (read_exact(archive, header, sizeof(header)))
            break;
        unsigned sum = 0;
        for (unsigned i = 0; i < sizeof(header); ++i)
            sum += header[i];
        if (!sum)
        {
            if (++zeros == 2)
            {
                result = 0;
                break;
            }
            continue;
        }
        if (zeros)
            break;
        uint64_t expected, bytes;
        if (octal(header + 148, 8, &expected) || octal(header + 124, 12, &bytes))
            break;
        for (unsigned i = 148; i < 156; ++i)
            sum = sum - header[i] + ' ';
        if (sum != expected)
            break;
        char name[PATH_CAPACITY], path[PATH_CAPACITY];
        if (safe_name(name, header))
            break;
        int count = snprintf(path, sizeof(path), "%s/%s", root, name);
        if (count < 0 || (unsigned)count >= sizeof(path))
            break;
        unsigned kind = header[156];
        if (kind != 0 && kind != '0' && kind != '5')
        {
            fputs("tar: unsupported archive entry\n", stderr);
            break;
        }
        if (kind == '5' && bytes)
            break;
        if (!extract)
            puts(name);
        int output = -1;
        if (extract)
        {
            if (parents(path, kind == '5'))
                break;
            if (kind != '5' && (output = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0)
                break;
        }
        int copied = transfer(archive, output, bytes);
        if (output >= 0 && close(output))
            copied = -1;
        if (copied || transfer(archive, -1, (512 - bytes % 512) % 512))
            break;
    }
    if (close(archive))
        result = 1;
    if (result)
        fputs("tar: archive read or extraction failed\n", stderr);
    return result;
}
