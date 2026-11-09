/* SPDX-License-Identifier: BSD-3-Clause */
#include "native_posix.h"
#include "lume/net_abi.h"
#include "lume/process.h"
#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/route.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static struct
{
    FILE *file;
    char *buffer;
} snapshots[16];

static int control(unsigned operation, struct lume_net_control *request)
{
    request->version = LUME_NET_CONTROL_VERSION;
    return native_result(
        native_call(LUME_SYS_NETCTL, operation, (uintptr_t)request, sizeof(*request)));
}

static int interfaces(struct lume_net_control *request)
{
    memset(request, 0, sizeof(*request));
    if (control(LUME_NET_ENUM, request) < 0)
        return -1;
    if (request->count > LUME_NET_INTERFACES)
    {
        errno = EIO;
        return -1;
    }
    return 0;
}

static int by_name(const char *name, struct lume_netif_info *info)
{
    struct lume_net_control request;
    if (interfaces(&request) < 0)
        return -1;
    for (unsigned i = 0; i < request.count; ++i)
        if (!strncmp(name, request.data.interfaces[i].name, 16))
        {
            *info = request.data.interfaces[i];
            return 0;
        }
    errno = ENODEV;
    return -1;
}

static const char *interface_name(uint32_t index, const struct lume_net_control *interfaces)
{
    for (unsigned i = 0; i < interfaces->count; ++i)
        if (interfaces->data.interfaces[i].ifindex == index)
            return interfaces->data.interfaces[i].name;
    return "?";
}

static uint32_t word(const uint8_t bytes[4])
{
    return (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8 | (uint32_t)bytes[2] << 16 |
           (uint32_t)bytes[3] << 24;
}

static int append(char *buffer, size_t *used, const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    int count = vsnprintf(buffer + *used, 16384 - *used, format, arguments);
    va_end(arguments);
    if (count < 0 || (size_t)count >= 16384 - *used)
    {
        errno = EOVERFLOW;
        return -1;
    }
    *used += count;
    return 0;
}

static int snapshot(const char *path, char *buffer, size_t *used)
{
    struct lume_net_control devices, request = {0};
    if (interfaces(&devices) < 0)
        return -1;
    if (!strcmp(path, "/proc/net/dev"))
    {
        append(buffer, used,
               "Inter-|   Receive                  |  Transmit\n"
               " face |packets errs drop fifo frame|packets errs drop fifo colls carrier\n");
        for (unsigned i = 0; i < devices.count; ++i)
        {
            const struct lume_netif_info *info = &devices.data.interfaces[i];
            if (append(buffer, used, "%s: %llu %llu 0 0 0 %llu %llu 0 0 0 0\n", info->name,
                       (unsigned long long)info->rx_packets, (unsigned long long)info->rx_errors,
                       (unsigned long long)info->tx_packets,
                       (unsigned long long)info->tx_errors) < 0)
                return -1;
        }
        return 0;
    }
    unsigned operation;
    if (!strcmp(path, "/proc/net/route"))
        operation = LUME_NET_ROUTES;
    else if (!strcmp(path, "/proc/net/arp"))
        operation = LUME_NET_NEIGHBORS;
    else if (!strcmp(path, "/proc/net/tcp") || !strcmp(path, "/proc/net/udp"))
        operation = LUME_NET_CONNECTIONS;
    else
    {
        errno = ENOTSUP;
        return -1;
    }
    if (control(operation, &request) < 0)
        return -1;
    if (request.count > LUME_NET_ROWS || request.lost)
    {
        errno = EOVERFLOW;
        return -1;
    }
    if (operation == LUME_NET_ROUTES)
    {
        append(buffer, used,
               "Iface Destination Gateway Flags RefCnt Use Metric Mask MTU Window IRTT\n");
        for (unsigned i = 0; i < request.count; ++i)
        {
            const struct lume_net_route *row = &request.data.routes[i];
            if (append(buffer, used, "%s %08X %08X %04X 0 0 %u %08X 0 0 0\n",
                       interface_name(row->ifindex, &devices), word(row->destination),
                       word(row->gateway), row->flags, row->metric, word(row->netmask)) < 0)
                return -1;
        }
    }
    else if (operation == LUME_NET_NEIGHBORS)
    {
        append(buffer, used, "IP address HW type Flags HW address Mask Device\n");
        for (unsigned i = 0; i < request.count; ++i)
        {
            const struct lume_net_neighbor *row = &request.data.neighbors[i];
            char address[16];
            inet_ntop(AF_INET, row->address, address, sizeof(address));
            if (append(buffer, used, "%s 0x1 0x%x %02x:%02x:%02x:%02x:%02x:%02x * %s\n", address,
                       row->flags, row->mac[0], row->mac[1], row->mac[2], row->mac[3], row->mac[4],
                       row->mac[5], interface_name(row->ifindex, &devices)) < 0)
                return -1;
        }
    }
    else
    {
        unsigned protocol = !strcmp(path, "/proc/net/tcp") ? 6 : 17;
        append(
            buffer, used,
            "sl local_address rem_address st tx_queue rx_queue tr tm->when retrnsmt uid timeout inode\n");
        for (unsigned i = 0; i < request.count; ++i)
        {
            const struct lume_net_connection *row = &request.data.connections[i];
            if (row->protocol != protocol)
                continue;
            struct lume_task_info owner = {0};
            int uid = -1;
            if (row->owner && native_call(LUME_SYS_PROCESS, row->owner - 1, (uintptr_t)&owner,
                                          sizeof(owner)) == 1)
                uid = (int)owner.uid;
            if (append(buffer, used,
                       "%u: %08X:%04X %08X:%04X %02X %08llX:%08llX 00:00000000 00000000 %d 0 %u\n",
                       i, word(row->local), row->local_port, word(row->remote), row->remote_port,
                       row->state, (unsigned long long)row->send_bytes,
                       (unsigned long long)row->receive_bytes, uid, row->handle) < 0)
                return -1;
        }
    }
    return 0;
}

FILE *lume_net_fopen(const char *path, const char *mode)
{
    if (strncmp(path, "/proc/net/", 10))
        return fopen(path, mode);
    if (strcmp(mode, "r"))
    {
        errno = EROFS;
        return NULL;
    }
    unsigned slot;
    for (slot = 0; slot < 16 && snapshots[slot].file; ++slot)
        ;
    if (slot == 16)
    {
        errno = EMFILE;
        return NULL;
    }
    char *buffer = malloc(16384);
    size_t used = 0;
    if (!buffer)
        return NULL;
    if (snapshot(path, buffer, &used) < 0)
    {
        free(buffer);
        return NULL;
    }
    FILE *file = fmemopen(buffer, used, "r");
    if (!file)
    {
        free(buffer);
        return NULL;
    }
    snapshots[slot].file = file;
    snapshots[slot].buffer = buffer;
    return file;
}

int lume_net_fclose(FILE *file)
{
    for (unsigned i = 0; i < 16; ++i)
        if (snapshots[i].file == file)
        {
            int result = fclose(file);
            free(snapshots[i].buffer);
            snapshots[i].file = NULL;
            snapshots[i].buffer = NULL;
            return result;
        }
    return fclose(file);
}

int lume_net_socket(int family, int type, int protocol)
{
    if (family != AF_INET || type != SOCK_DGRAM || protocol)
    {
        errno = EAFNOSUPPORT;
        return -1;
    }
    return socket(AF_INET, SOCK_STREAM, 0);
}

static void address(struct sockaddr *out, const uint8_t bytes[4])
{
    struct sockaddr_in result = {.sin_family = AF_INET};
    memcpy(&result.sin_addr.s_addr, bytes, 4);
    memcpy(out, &result, sizeof(*out));
}

static unsigned flags(const struct lume_netif_info *info)
{
    return IFF_BROADCAST | (info->flags & LUME_IF_UP ? IFF_UP : 0) |
           (info->flags & LUME_IF_LINK ? IFF_RUNNING : 0);
}

int lume_net_ioctl(int fd, unsigned long operation, ...)
{
    va_list arguments;
    va_start(arguments, operation);
    void *argument = va_arg(arguments, void *);
    va_end(arguments);
    (void)fd;
    if (!argument)
    {
        errno = EFAULT;
        return -1;
    }
    if (operation == SIOCGIFCONF)
    {
        struct ifconf *list = argument;
        struct lume_net_control request;
        if (interfaces(&request) < 0)
            return -1;
        unsigned capacity = list->ifc_len > 0 ? (unsigned)list->ifc_len / sizeof(struct ifreq) : 0;
        unsigned count = list->ifc_buf ? (capacity < request.count ? capacity : request.count) : 0;
        for (unsigned i = 0; i < count; ++i)
        {
            memset(&list->ifc_req[i], 0, sizeof(struct ifreq));
            memcpy(list->ifc_req[i].ifr_name, request.data.interfaces[i].name, IFNAMSIZ);
            address(&list->ifc_req[i].ifr_addr, request.data.interfaces[i].address);
        }
        list->ifc_len = (list->ifc_buf ? count : request.count) * sizeof(struct ifreq);
        return 0;
    }
    if (operation == SIOCADDRT || operation == SIOCDELRT)
    {
        const struct rtentry *route = argument;
        struct lume_net_control devices, request = {0};
        if (interfaces(&devices) < 0)
            return -1;
        memcpy(request.data.routes[0].destination,
               &((const struct sockaddr_in *)&route->rt_dst)->sin_addr, 4);
        memcpy(request.data.routes[0].netmask,
               &((const struct sockaddr_in *)&route->rt_genmask)->sin_addr, 4);
        memcpy(request.data.routes[0].gateway,
               &((const struct sockaddr_in *)&route->rt_gateway)->sin_addr, 4);
        request.data.routes[0].flags = route->rt_flags;
        request.data.routes[0].metric = route->rt_metric > 0 ? route->rt_metric - 1 : 0;
        if (operation == SIOCDELRT && !route->rt_dev && !word(request.data.routes[0].gateway))
        {
            struct lume_net_control table = {0};
            if (control(LUME_NET_ROUTES, &table) < 0)
                return -1;
            if (table.count > LUME_NET_ROWS)
            {
                errno = EIO;
                return -1;
            }
            for (unsigned i = 0; i < table.count; ++i)
                if ((table.data.routes[i].flags & RTF_GATEWAY) &&
                    !word(table.data.routes[i].destination) && !word(table.data.routes[i].netmask))
                {
                    request.ifindex = table.data.routes[i].ifindex;
                    break;
                }
            if (!request.ifindex)
            {
                errno = ESRCH;
                return -1;
            }
        }
        for (unsigned i = 0; i < devices.count; ++i)
        {
            if (request.ifindex)
                break;
            const struct lume_netif_info *info = &devices.data.interfaces[i];
            if (route->rt_dev ? !strcmp(route->rt_dev, info->name)
                              : ((word(request.data.routes[0].gateway) & word(info->netmask)) ==
                                 (word(info->address) & word(info->netmask))))
            {
                request.ifindex = info->ifindex;
                break;
            }
        }
        if (!request.ifindex)
        {
            errno = ENODEV;
            return -1;
        }
        return control(operation == SIOCADDRT ? LUME_NET_ROUTE_ADD : LUME_NET_ROUTE_DELETE,
                       &request);
    }
    if (operation == SIOCSARP || operation == SIOCDARP || operation == SIOCGARP)
    {
        errno = ENOTSUP;
        return -1;
    }
    struct ifreq *input = argument;
    struct lume_netif_info info;
    if (by_name(input->ifr_name, &info) < 0)
        return -1;
    struct lume_net_control request = {.ifindex = info.ifindex};
    switch (operation)
    {
    case SIOCGIFFLAGS:
        input->ifr_flags = flags(&info);
        return 0;
    case SIOCGIFADDR:
        address(&input->ifr_addr, info.address);
        return 0;
    case SIOCGIFNETMASK:
        address(&input->ifr_netmask, info.netmask);
        return 0;
    case SIOCGIFBRDADDR:
    {
        uint8_t broadcast[4];
        for (unsigned i = 0; i < 4; ++i)
            broadcast[i] = info.address[i] | (uint8_t)~info.netmask[i];
        address(&input->ifr_broadaddr, broadcast);
        return 0;
    }
    case SIOCGIFHWADDR:
        memset(&input->ifr_hwaddr, 0, sizeof(input->ifr_hwaddr));
        input->ifr_hwaddr.sa_family = ARPHRD_ETHER;
        memcpy(input->ifr_hwaddr.sa_data, info.mac, 6);
        return 0;
    case SIOCGIFMTU:
        input->ifr_mtu = info.mtu;
        return 0;
    case SIOCGIFINDEX:
        input->ifr_ifindex = info.ifindex;
        return 0;
    case SIOCGIFMETRIC:
        input->ifr_metric = 0;
        return 0;
    case SIOCGIFMAP:
        memset(&input->ifr_map, 0, sizeof(input->ifr_map));
        return 0;
    case SIOCSIFFLAGS:
        if ((input->ifr_flags ^ flags(&info)) & ~(IFF_UP | IFF_RUNNING))
            break;
        request.data.admin_up = (input->ifr_flags & IFF_UP) != 0;
        return control(LUME_NET_ADMIN, &request);
    case SIOCSIFADDR:
    case SIOCSIFNETMASK:
        if (input->ifr_addr.sa_family != AF_INET)
            break;
        request.data.ipv4.fields =
            operation == SIOCSIFADDR ? LUME_NET_IPV4_ADDRESS : LUME_NET_IPV4_MASK;
        memcpy(operation == SIOCSIFADDR ? request.data.ipv4.address : request.data.ipv4.netmask,
               &((const struct sockaddr_in *)&input->ifr_addr)->sin_addr, 4);
        return control(LUME_NET_IPV4, &request);
    }
    errno = ENOTSUP;
    return -1;
}
