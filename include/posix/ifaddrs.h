#ifndef LUME_POSIX_IFADDRS_H
#define LUME_POSIX_IFADDRS_H

#include <sys/socket.h>

struct ifaddrs
{
    struct ifaddrs *ifa_next;
    char *ifa_name;
    unsigned ifa_flags;
    struct sockaddr *ifa_addr;
    struct sockaddr *ifa_netmask;

    union
    {
        struct sockaddr *broadcast;
        struct sockaddr *destination;
    } ifa_ifu;

    void *ifa_data;
};

#define ifa_broadaddr ifa_ifu.broadcast
#define ifa_dstaddr ifa_ifu.destination

#ifdef __cplusplus
extern "C"
{
#endif
    int getifaddrs(struct ifaddrs **);
    void freeifaddrs(struct ifaddrs *);
#ifdef __cplusplus
}
#endif

#endif
