#include "config.h"
#include "net_client.h"
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

int inet_pton(int family, const char *s, void *out)
{
    if (family != AF_INET)
    {
        errno = EAFNOSUPPORT;
        return -1;
    }
    return net_parse_ip(s, out);
}

const char *inet_ntop(int family, const void *address, char *text, socklen_t size)
{
    if (family != AF_INET)
    {
        errno = EAFNOSUPPORT;
        return NULL;
    }
    const unsigned char *p = address;
    int n = snprintf(text, size, "%u.%u.%u.%u", p[0], p[1], p[2], p[3]);
    if (n < 0 || (unsigned)n >= size)
    {
        errno = ENOSPC;
        return NULL;
    }
    return text;
}

in_addr_t inet_addr(const char *s)
{
    in_addr_t value;
    return inet_pton(AF_INET, s, &value) == 1 ? value : INADDR_NONE;
}

int getaddrinfo(const char *node, const char *service, const struct addrinfo *hint,
                struct addrinfo **out)
{
    *out = NULL;
    if (hint && hint->ai_family != AF_INET && hint->ai_family != AF_UNSPEC)
        return EAI_FAMILY;
    char *end;
    unsigned long port = service ? strtoul(service, &end, 10) : 0;
    if (service && (!*service || *end || port > 65535))
        return EAI_NONAME;
    struct addrinfo *a = calloc(1, sizeof *a);
    struct sockaddr_in *s = calloc(1, sizeof *s);
    if (!a || !s)
    {
        free(a);
        free(s);
        return EAI_MEMORY;
    }
    s->sin_family = AF_INET;
    s->sin_port = htons(port);
    if (node && *node && inet_pton(AF_INET, node, &s->sin_addr) != 1)
    {
        free(a);
        free(s);
        return EAI_NONAME;
    }
    a->ai_family = AF_INET;
    a->ai_socktype = SOCK_STREAM;
    a->ai_protocol = IPPROTO_TCP;
    a->ai_addrlen = sizeof *s;
    a->ai_addr = (void *)s;
    *out = a;
    return 0;
}

void freeaddrinfo(struct addrinfo *a)
{
    while (a)
    {
        struct addrinfo *next = a->ai_next;
        free(a->ai_addr);
        free(a->ai_canonname);
        free(a);
        a = next;
    }
}

const char *gai_strerror(int n)
{
    (void)n;
    return "Lume supports numeric IPv4 addresses";
}

int getnameinfo(const struct sockaddr *a, socklen_t size, char *host, socklen_t hn, char *service,
                socklen_t sn, int flags)
{
    (void)flags;
    if (size < sizeof(struct sockaddr_in) || a->sa_family != AF_INET)
        return EAI_FAMILY;
    const struct sockaddr_in *s = (const void *)a;
    if (host && !inet_ntop(AF_INET, &s->sin_addr, host, hn))
        return EAI_NONAME;
    if (service)
    {
        int n = snprintf(service, sn, "%u", ntohs(s->sin_port));
        if (n < 0 || (unsigned)n >= sn)
            return EAI_NONAME;
    }
    return 0;
}
