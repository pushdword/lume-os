#ifndef LUME_NET_TOOLS_ARP_H
#define LUME_NET_TOOLS_ARP_H
#include <sys/socket.h>
#define ARPHRD_ETHER 1
#define ARPHRD_LOOPBACK 772
#define ATF_COM 2
#define ATF_PERM 4
#define ATF_PUBL 8
#define ATF_USETRAILERS 16
#define ATF_NETMASK 32
#define ATF_DONTPUB 64

struct arpreq
{
    struct sockaddr arp_pa, arp_ha;
    int arp_flags;
    struct sockaddr arp_netmask;
    char arp_dev[16];
};
#endif
