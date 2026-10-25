#ifndef LUME_SSH_NETINET_H
#define LUME_SSH_NETINET_H
#include <sys/socket.h>
typedef uint32_t in_addr_t;
typedef uint16_t in_port_t;

struct in_addr
{
    in_addr_t s_addr;
};

struct sockaddr_in
{
    sa_family_t sin_family;
    in_port_t sin_port;
    struct in_addr sin_addr;
    unsigned char sin_zero[8];
};

struct in6_addr
{
    unsigned char s6_addr[16];
};

struct sockaddr_in6
{
    sa_family_t sin6_family;
    in_port_t sin6_port;
    uint32_t sin6_flowinfo;
    struct in6_addr sin6_addr;
    uint32_t sin6_scope_id;
};

#define INADDR_ANY 0u
#define INADDR_LOOPBACK 0x7f000001u
#define INADDR_NONE 0xffffffffu
#define IPPROTO_IP 0
#define IPPROTO_TCP 6
#define IPPROTO_UDP 17
#define IP_TOS 1
#define IPV6_V6ONLY 26
#define INET_ADDRSTRLEN 16
#define INET6_ADDRSTRLEN 46

static inline uint16_t htons(uint16_t v)
{
    return (uint16_t)((v << 8) | (v >> 8));
}

static inline uint32_t htonl(uint32_t v)
{
    return __builtin_bswap32(v);
}

#define ntohs htons
#define ntohl htonl
#define IPPORT_RESERVED 1024
#define IPPROTO_IPV6 41
#endif
