#ifndef LUME_SYSCTL_CONFIG_H
#define LUME_SYSCTL_CONFIG_H
#include "api.h"
#include <string.h>

static const struct
{
    const char *key, *attribute;
} sysctl_keys[] = {{"fs.file-max", "file-max"},
                   {"fs.file-nr", "file-nr"},
                   {"fs.nr-open", "nofile-max"},
                   {"fs.nofile-default", "nofile-default"},
                   {"fs.pipe-pages-max", "pipe-pages-max"},
                   {"fs.pipe-pages-used", "pipe-pages-used"}};

static char *sysctl_trim(char *text)
{
    while (*text == ' ' || *text == '\t' || *text == '\r')
        ++text;
    size_t bytes = strlen(text);
    while (bytes && (text[bytes - 1] == ' ' || text[bytes - 1] == '\t' || text[bytes - 1] == '\r'))
        text[--bytes] = 0;
    return text;
}

static int sysctl_value(const char *key, const char *value, int display)
{
    unsigned index = 0;
    while (index < sizeof(sysctl_keys) / sizeof(sysctl_keys[0]) &&
           strcmp(key, sysctl_keys[index].key))
        ++index;
    if (index == sizeof(sysctl_keys) / sizeof(sysctl_keys[0]))
    {
        app_puts("sysctl: unknown key: ");
        app_puts(key);
        app_puts("\n");
        return 1;
    }
    char path[64] = "/dev/limits/";
    strcat(path, sysctl_keys[index].attribute);
    int fd = app_open(path, value ? LUME_WRONLY : LUME_RDONLY);
    if (fd < 0)
    {
        app_error(key, fd);
        return 1;
    }
    char data[64];
    int result;
    if (value)
    {
        size_t bytes = strlen(value);
        result = bytes && bytes < sizeof(data) ? app_write(fd, value, (unsigned)bytes) : -22;
        if (result >= 0 && (size_t)result != bytes)
            result = -5;
    }
    else
    {
        result = app_read(fd, data, sizeof(data) - 1);
        if (result >= 0)
            data[result] = 0;
    }
    int closed = app_close(fd);
    if (result >= 0 && closed < 0)
        result = closed;
    if (result < 0)
    {
        app_error(key, result);
        return 1;
    }
    if (display)
    {
        app_puts(key);
        app_puts(" = ");
        app_puts(value ? value : data);
        if (value || !result || data[result - 1] != '\n')
            app_puts("\n");
    }
    return 0;
}

static int sysctl_assignment(char *line, int display)
{
    char *comment = strchr(line, '#');
    if (comment)
        *comment = 0;
    line = sysctl_trim(line);
    if (!*line || *line == ';')
        return 0;
    char *value = strchr(line, '=');
    if (!value)
    {
        app_puts("sysctl: expected KEY=VALUE\n");
        return 1;
    }
    *value++ = 0;
    return sysctl_value(sysctl_trim(line), sysctl_trim(value), display);
}

/* Configuration is optional at boot. Invalid entries are reported and later entries
 * are still processed. A failed write leaves the previous kernel limit in effect. */
static int lume_sysctl_load(const char *path, int optional, int display)
{
    int fd = app_open(path, LUME_RDONLY);
    if (fd < 0)
    {
        if (optional && fd == -2)
            return 0;
        app_error(path, fd);
        return 1;
    }
    char input[512], line[128];
    unsigned used = 0;
    int failed = 0, overflow = 0, count;
    while ((count = app_read(fd, input, sizeof(input))) > 0)
        for (int i = 0; i < count; ++i)
        {
            if (input[i] == '\n')
            {
                line[used] = 0;
                if (overflow)
                {
                    app_puts("sysctl: configuration line too long\n");
                    failed = 1;
                }
                else
                    failed |= sysctl_assignment(line, display);
                used = 0;
                overflow = 0;
            }
            else if (!input[i] || used == sizeof(line) - 1)
                overflow = 1;
            else if (!overflow)
                line[used++] = input[i];
        }
    if (count < 0)
    {
        app_error(path, count);
        failed = 1;
    }
    if (overflow)
    {
        app_puts("sysctl: configuration line too long\n");
        failed = 1;
    }
    else if (used)
    {
        line[used] = 0;
        failed |= sysctl_assignment(line, display);
    }
    if (app_close(fd) < 0)
        failed = 1;
    return failed;
}

#endif
