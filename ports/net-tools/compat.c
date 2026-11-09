/* SPDX-License-Identifier: BSD-3-Clause */
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int inet_aton(const char *text, struct in_addr *address)
{
    return inet_pton(AF_INET, text, address) == 1;
}

struct netent *getnetbyname(const char *name)
{
    (void)name;
    return NULL;
}

struct netent *getnetbyaddr(uint32_t address, int family)
{
    (void)address;
    (void)family;
    return NULL;
}

static struct protoent protocols[] = {
    {.p_name = "icmp", .p_proto = 1},
    {.p_name = "tcp", .p_proto = 6},
    {.p_name = "udp", .p_proto = 17},
};
static unsigned protocol_index, service_index;

static const struct
{
    const char *name, *protocol;
    unsigned port;
} services[] = {{"ssh", "tcp", 22},
                {"domain", "udp", 53},
                {"domain", "tcp", 53},
                {"http", "tcp", 80},
                {"https", "tcp", 443}};

void setprotoent(int stay)
{
    (void)stay;
    protocol_index = 0;
}

void endprotoent(void)
{
    protocol_index = 0;
}

struct protoent *getprotoent(void)
{
    return protocol_index < sizeof(protocols) / sizeof(*protocols) ? &protocols[protocol_index++]
                                                                   : NULL;
}

struct protoent *getprotobyname(const char *name)
{
    for (unsigned i = 0; i < sizeof(protocols) / sizeof(*protocols); ++i)
        if (!strcmp(name, protocols[i].p_name))
            return &protocols[i];
    return NULL;
}

struct protoent *getprotobynumber(int number)
{
    for (unsigned i = 0; i < sizeof(protocols) / sizeof(*protocols); ++i)
        if (number == protocols[i].p_proto)
            return &protocols[i];
    return NULL;
}

void setservent(int stay)
{
    (void)stay;
    service_index = 0;
}

void endservent(void)
{
    service_index = 0;
}

struct servent *getservent(void)
{
    static struct servent service;
    static char *aliases[] = {NULL};
    if (service_index == sizeof(services) / sizeof(*services))
        return NULL;
    service.s_name = (char *)services[service_index].name;
    service.s_proto = (char *)services[service_index].protocol;
    service.s_port = htons(services[service_index++].port);
    service.s_aliases = aliases;
    return &service;
}

struct servent *getservbyport(int port, const char *protocol)
{
    setservent(0);
    struct servent *service;
    while ((service = getservent()))
        if (service->s_port == port && (!protocol || !strcmp(protocol, service->s_proto)))
            return service;
    return NULL;
}

const char *hstrerror(int error)
{
    return error == TRY_AGAIN ? "Temporary name resolution failure" : "Host not found";
}

void herror(const char *name)
{
    fprintf(stderr, "%s: %s\n", name, hstrerror(h_errno));
}

int getdomainname(char *name, size_t bytes)
{
    if (!bytes)
    {
        errno = ENAMETOOLONG;
        return -1;
    }
    *name = 0;
    return 0;
}

int sethostname(const char *name, size_t bytes)
{
    (void)name;
    (void)bytes;
    errno = ENOTSUP;
    return -1;
}

int setdomainname(const char *name, size_t bytes)
{
    (void)name;
    (void)bytes;
    errno = ENOTSUP;
    return -1;
}
