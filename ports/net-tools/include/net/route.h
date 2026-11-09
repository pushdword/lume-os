#ifndef LUME_NET_TOOLS_ROUTE_H
#define LUME_NET_TOOLS_ROUTE_H
#include <sys/socket.h>
#define RTF_UP 1
#define RTF_GATEWAY 2
#define RTF_HOST 4
#define RTF_REINSTATE 8
#define RTF_DYNAMIC 16
#define RTF_MODIFIED 32
#define RTF_MTU 64
#define RTF_MSS RTF_MTU
#define RTF_WINDOW 128
#define RTF_IRTT 256
#define RTF_REJECT 512

struct rtentry
{
    unsigned long rt_pad1;
    struct sockaddr rt_dst, rt_gateway, rt_genmask;
    unsigned short rt_flags;
    short rt_pad2;
    unsigned long rt_pad3;
    void *rt_pad4;
    short rt_metric;
    char *rt_dev;
    unsigned long rt_mtu, rt_window;
    unsigned short rt_irtt;
};

#define rt_mss rt_mtu
#endif
