#ifndef LUME_NET_TOOLS_IF_H
#define LUME_NET_TOOLS_IF_H
#include_next <net/if.h>
#include <sys/socket.h>
#define IFF_BROADCAST 2
#define IFF_DEBUG 4
#define IFF_LOOPBACK 8
#define IFF_POINTOPOINT 16
#define IFF_NOTRAILERS 32
#define IFF_NOARP 128
#define IFF_PROMISC 256
#define IFF_ALLMULTI 512
#define IFF_MASTER 1024
#define IFF_SLAVE 2048
#define IFF_MULTICAST 4096

struct ifmap
{
    unsigned long mem_start, mem_end;
    unsigned short base_addr;
    unsigned char irq, dma, port;
};

struct ifreq
{
    char ifr_name[IFNAMSIZ];

    union
    {
        struct sockaddr addr;
        short flags;
        int value;
        struct ifmap map;
        char name[IFNAMSIZ];
        void *data;
    } ifr_ifru;
};

#define ifr_addr ifr_ifru.addr
#define ifr_dstaddr ifr_ifru.addr
#define ifr_broadaddr ifr_ifru.addr
#define ifr_netmask ifr_ifru.addr
#define ifr_hwaddr ifr_ifru.addr
#define ifr_flags ifr_ifru.flags
#define ifr_mtu ifr_ifru.value
#define ifr_metric ifr_ifru.value
#define ifr_ifindex ifr_ifru.value
#define ifr_qlen ifr_ifru.value
#define ifr_map ifr_ifru.map
#define ifr_data ifr_ifru.data
#define ifr_newname ifr_ifru.name

struct ifconf
{
    int ifc_len;

    union
    {
        char *buf;
        struct ifreq *req;
    } ifc_ifcu;
};

#define ifc_buf ifc_ifcu.buf
#define ifc_req ifc_ifcu.req
#endif
