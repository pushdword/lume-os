#ifndef LUME_POSIX_NETINET_H
#define LUME_POSIX_NETINET_H
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
#define IP_TTL 2
#define IP_RECVTOS 13
#define IP_MULTICAST_IF 32
#define IP_MULTICAST_TTL 33
#define IP_MULTICAST_LOOP 34
#define IP_ADD_MEMBERSHIP 35
#define IP_DROP_MEMBERSHIP 36
#define IPV6_UNICAST_HOPS 16
#define IPV6_MULTICAST_IF 17
#define IPV6_MULTICAST_HOPS 18
#define IPV6_MULTICAST_LOOP 19
#define IPV6_JOIN_GROUP 20
#define IPV6_LEAVE_GROUP 21
#define IPV6_V6ONLY 26
#define IPV6_TCLASS 67
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
#define IN_MULTICAST(address) (((uint32_t)(address) & UINT32_C(0xf0000000)) == UINT32_C(0xe0000000))
#define IN6ADDR_ANY_INIT {{0}}
#define IN6ADDR_LOOPBACK_INIT {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}}

struct ip_mreq
{
    struct in_addr imr_multiaddr, imr_interface;
};

struct ipv6_mreq
{
    struct in6_addr ipv6mr_multiaddr;
    unsigned ipv6mr_interface;
};

#ifdef __cplusplus
extern "C"
{
#endif
    extern const struct in6_addr in6addr_any, in6addr_loopback;
#ifdef __cplusplus
}
#endif

static inline int lume_in6_unspecified(const struct in6_addr *address)
{
    for (unsigned i = 0; i < 16; ++i)
        if (address->s6_addr[i])
            return 0;
    return 1;
}

static inline int lume_in6_multicast_scope(const struct in6_addr *address, unsigned scope)
{
    return address->s6_addr[0] == 0xff && (address->s6_addr[1] & 15) == scope;
}

#define IN6_IS_ADDR_UNSPECIFIED(address) lume_in6_unspecified(address)
#define IN6_IS_ADDR_LINKLOCAL(address)                                                             \
    ((address)->s6_addr[0] == 0xfe && ((address)->s6_addr[1] & 0xc0) == 0x80)
#define IN6_IS_ADDR_SITELOCAL(address)                                                             \
    ((address)->s6_addr[0] == 0xfe && ((address)->s6_addr[1] & 0xc0) == 0xc0)
#define IN6_IS_ADDR_MULTICAST(address) ((address)->s6_addr[0] == 0xff)
#define IN6_IS_ADDR_MC_NODELOCAL(address) lume_in6_multicast_scope(address, 1)
#define IN6_IS_ADDR_MC_LINKLOCAL(address) lume_in6_multicast_scope(address, 2)
#define IN6_IS_ADDR_MC_SITELOCAL(address) lume_in6_multicast_scope(address, 5)
#define IN6_IS_ADDR_MC_ORGLOCAL(address) lume_in6_multicast_scope(address, 8)
#define IN6_IS_ADDR_MC_GLOBAL(address) lume_in6_multicast_scope(address, 14)

static inline int lume_in6_v4mapped(const struct in6_addr *address)
{
    for (unsigned i = 0; i < 10; ++i)
        if (address->s6_addr[i])
            return 0;
    return address->s6_addr[10] == 255 && address->s6_addr[11] == 255;
}

static inline int lume_in6_loopback(const struct in6_addr *address)
{
    for (unsigned i = 0; i < 15; ++i)
        if (address->s6_addr[i])
            return 0;
    return address->s6_addr[15] == 1;
}

#define IN6_IS_ADDR_V4MAPPED(address) lume_in6_v4mapped(address)
#define IN6_IS_ADDR_LOOPBACK(address) lume_in6_loopback(address)
#endif
