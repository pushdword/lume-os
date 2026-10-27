#ifndef LUME_NETWORK_TOOL_H
#define LUME_NETWORK_TOOL_H
#include "../../include/internal/lume/app_support.h"
#include "lume/io.h"
#include "lume/net_commands.h"
#include <stdbool.h>

static void tool_text(unsigned fd, const char *text)
{
    (void)write_all((int)fd, text, len(text));
}

static void tool_number(unsigned fd, uint64_t value)
{
    char digits[21];
    unsigned n = 0;
    do
    {
        digits[20 - n++] = (char)('0' + value % 10);
        value /= 10;
    } while (value);
    (void)write_all((int)fd, digits + 21 - n, n);
}

static bool tool_unsigned(const char *s, unsigned limit, unsigned *result)
{
    unsigned n = 0;
    if (!*s)
        return false;
    while (*s)
    {
        unsigned digit = (unsigned char)*s++ - '0';
        if (digit > 9 || digit > limit || n > (limit - digit) / 10)
            return false;
        n = n * 10 + digit;
    }
    *result = n;
    return true;
}

static bool tool_address(const char *s, unsigned char address[4])
{
    for (unsigned i = 0; i < 4; ++i)
    {
        unsigned value = 0, count = 0;
        while (*s >= '0' && *s <= '9')
        {
            value = value * 10 + (unsigned)(*s++ - '0');
            if (++count > 3 || value > 255)
                return false;
        }
        if (!count || (i < 3 ? *s++ != '.' : *s != 0))
            return false;
        address[i] = (unsigned char)value;
    }
    return true;
}

static void tool_ip(unsigned fd, const unsigned char address[4])
{
    for (unsigned i = 0; i < 4; ++i)
    {
        if (i)
            tool_text(fd, ".");
        tool_number(fd, address[i]);
    }
}

static bool tool_resolve(const char *name, unsigned char address[4], bool numeric)
{
    if (tool_address(name, address))
        return true;
    struct lume_net_request request = {0};
    size_t count = len(name);
    if (numeric || !count || count >= sizeof(request.argument))
        return false;
    for (size_t i = 0; i <= count; ++i)
        request.argument[i] = name[i];
    if (call(LUME_SYS_NET, LUME_NET_DNS, (uintptr_t)&request, sizeof(request)) < 0)
        return false;
    for (unsigned i = 0; i < sizeof(request.output); ++i)
        if (request.output[i] == '\n')
        {
            request.output[i] = 0;
            return tool_address(request.output, address);
        }
    return false;
}

static uint64_t tool_now(void)
{
    return (uint64_t)call(LUME_SYS_CLOCK, 0, 0, 0);
}

static void tool_sleep(unsigned ms)
{
    while (ms)
    {
        unsigned count = ms > 60000 ? 60000 : ms;
        (void)call(LUME_SYS_SLEEP, count, 0, 0);
        ms -= count;
    }
}
#endif
