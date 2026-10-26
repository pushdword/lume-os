/* Small nonblocking network wrappers for Lume applications. */
#ifndef LUME_NET_CLIENT_H
#define LUME_NET_CLIENT_H
#include "api.h"
#include "net.h"
#include <string.h>

static inline uint32_t net_now(void)
{
    return (uint32_t)lume_syscall(LUME_INFO, LUME_UPTIME, 0, 0);
}

static inline void net_idle(unsigned ms)
{
    (void)lume_syscall(ms ? LUME_SLEEP : LUME_YIELD, ms, 0, 0);
}

static inline struct lume_net_request net_request(int handle)
{
    struct lume_net_request r = {0};
    r.version = LUME_NET_VERSION;
    r.bytes = sizeof r;
    r.handle = handle;
    return r;
}

static inline int net_call(unsigned op, struct lume_net_request *r)
{
    return lume_syscall(LUME_NET, op, (uint32_t)(uintptr_t)r, sizeof *r);
}

static inline int net_socket(unsigned protocol)
{
    struct lume_net_request r = net_request(-1);
    r.protocol = protocol;
    return net_call(LUME_NET_SOCKET, &r);
}

static inline int net_close(int handle)
{
    struct lume_net_request r = net_request(handle);
    return net_call(LUME_NET_CLOSE, &r);
}

static inline int net_bind(int handle, unsigned port)
{
    struct lume_net_request r = net_request(handle);
    r.port = port;
    return net_call(LUME_NET_BIND, &r);
}

static inline int net_listen(int handle)
{
    struct lume_net_request r = net_request(handle);
    return net_call(LUME_NET_LISTEN, &r);
}

static inline int net_info(struct lume_net_info *info)
{
    struct lume_net_request r = net_request(-1);
    r.buffer = (uint32_t)(uintptr_t)info;
    r.length = sizeof *info;
    return net_call(LUME_NET_INFO, &r);
}

static inline int net_config(struct lume_net_config *c)
{
    struct lume_net_request r = net_request(-1);
    r.buffer = (uint32_t)(uintptr_t)c;
    r.length = sizeof *c;
    return net_call(LUME_NET_CONFIG, &r);
}

static inline int net_io(unsigned op, int handle, void *data, unsigned length)
{
    struct lume_net_request r = net_request(handle);
    r.buffer = (uint32_t)(uintptr_t)data;
    r.length = length;
    return net_call(op, &r);
}

static inline void net_ip(const unsigned char *ip)
{
    for (unsigned i = 0; i < 4; i++)
    {
        if (i)
            app_puts(".");
        app_number(ip[i]);
    }
}

static inline int net_parse_ip(const char *s, unsigned char *ip)
{
    for (unsigned i = 0; i < 4; i++)
    {
        unsigned n = 0, digits = 0;
        while (*s >= '0' && *s <= '9')
        {
            n = n * 10u + (unsigned)(*s++ - '0');
            if (n > 255 || ++digits > 3)
                return 0;
        }
        if (!digits)
            return 0;
        ip[i] = (unsigned char)n;
        if (i < 3)
        {
            if (*s++ != '.')
                return 0;
        }
        else if (*s)
            return 0;
    }
    return 1;
}
#endif
