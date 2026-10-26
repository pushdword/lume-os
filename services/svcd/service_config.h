#ifndef LUME_SERVICE_CONFIG_H
#define LUME_SERVICE_CONFIG_H
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define SERVICE_ARGUMENTS 16u
#define SERVICE_TEXT_BYTES 4096u

enum
{
    SERVICE_RESTART_NEVER,
    SERVICE_RESTART_FAILURE,
    SERVICE_RESTART_ALWAYS
};

struct service_config
{
    char program[128], argument[SERVICE_ARGUMENTS - 1][128];
    unsigned arguments, restart, delay_ms;
};

static bool service_path(const char *path)
{
    if (*path++ != '/' || !*path)
        return false;
    for (const char *component = path;; ++path)
    {
        unsigned c = (unsigned char)*path;
        if (c && c < 32)
            return false;
        if (!c || c == '/')
        {
            size_t n = (size_t)(path - component);
            if (!n || (n == 1 && component[0] == '.') ||
                (n == 2 && component[0] == '.' && component[1] == '.'))
                return false;
            if (!c)
                return true;
            component = path + 1;
        }
    }
}

static bool service_config_parse(struct service_config *output, const char *text, size_t bytes)
{
    struct service_config config = {.restart = SERVICE_RESTART_FAILURE, .delay_ms = 1000};
    bool executable = false, restart = false, delay = false;
    if (!bytes || bytes > SERVICE_TEXT_BYTES)
        return false;
    size_t at = 0;
    while (at < bytes)
    {
        char line[160];
        unsigned n = 0;
        while (at < bytes && text[at] != '\n')
        {
            unsigned c = (unsigned char)text[at++];
            if (!c || n + 1 >= sizeof(line))
                return false;
            line[n++] = (char)c;
        }
        if (at < bytes)
            ++at;
        if (n && line[n - 1] == '\r')
            --n;
        line[n] = 0;
        if (!n || line[0] == '#')
            continue;
        char *value = strchr(line, '=');
        if (!value)
            return false;
        *value++ = 0;
        size_t length = strlen(value);
        if (!strcmp(line, "exec"))
        {
            if (executable || length >= sizeof(config.program) || !service_path(value))
                return false;
            memcpy(config.program, value, length + 1);
            executable = true;
        }
        else if (!strcmp(line, "arg"))
        {
            if (config.arguments == SERVICE_ARGUMENTS - 1 || length >= sizeof(config.argument[0]))
                return false;
            memcpy(config.argument[config.arguments++], value, length + 1);
        }
        else if (!strcmp(line, "restart"))
        {
            if (restart)
                return false;
            restart = true;
            if (!strcmp(value, "never"))
                config.restart = SERVICE_RESTART_NEVER;
            else if (!strcmp(value, "on-failure"))
                config.restart = SERVICE_RESTART_FAILURE;
            else if (!strcmp(value, "always"))
                config.restart = SERVICE_RESTART_ALWAYS;
            else
                return false;
        }
        else if (!strcmp(line, "restart-delay-ms"))
        {
            if (delay || !length)
                return false;
            delay = true;
            unsigned number = 0;
            for (const char *p = value; *p; ++p)
            {
                if (*p < '0' || *p > '9' || number > 60000 / 10)
                    return false;
                number = number * 10 + (unsigned)(*p - '0');
                if (number > 60000)
                    return false;
            }
            if (number < 100)
                return false;
            config.delay_ms = number;
        }
        else
            return false;
    }
    if (!executable)
        return false;
    *output = config;
    return true;
}
#endif
