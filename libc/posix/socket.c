#include "native_posix.h"
#include <sys/un.h>
#include "lume/socket.h"
#include "fd.h"
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <net/if.h>
#include <ifaddrs.h>
#include "lume/net_abi.h"
#include "lume/net_commands.h"

const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
const struct in6_addr in6addr_loopback = IN6ADDR_LOOPBACK_INIT;

struct if_nameindex *if_nameindex(void)
{
    struct lume_net_control request = {.version = LUME_NET_CONTROL_VERSION};
    if (native_result(
            native_call(LUME_SYS_NETCTL, LUME_NET_ENUM, (uintptr_t)&request, sizeof(request))) < 0)
        return NULL;
    if (request.count > LUME_NET_INTERFACES)
    {
        errno = EIO;
        return NULL;
    }
    struct if_nameindex *result = calloc(request.count + 1, sizeof(*result));
    if (!result)
        return NULL;
    for (unsigned i = 0; i < request.count; ++i)
    {
        result[i].if_index = request.data.interfaces[i].ifindex;
        result[i].if_name = strndup(request.data.interfaces[i].name, 16);
        if (!result[i].if_name)
        {
            if_freenameindex(result);
            return NULL;
        }
    }
    return result;
}

void if_freenameindex(struct if_nameindex *interfaces)
{
    if (!interfaces)
        return;
    for (unsigned i = 0; interfaces[i].if_index; ++i)
        free(interfaces[i].if_name);
    free(interfaces);
}

unsigned if_nametoindex(const char *name)
{
    if (!name || !*name || strnlen(name, IF_NAMESIZE) == IF_NAMESIZE)
    {
        errno = ENXIO;
        return 0;
    }
    struct if_nameindex *interfaces = if_nameindex();
    if (!interfaces)
        return 0;
    unsigned index = 0;
    for (struct if_nameindex *entry = interfaces; entry->if_index; ++entry)
        if (!strcmp(entry->if_name, name))
        {
            index = entry->if_index;
            break;
        }
    if_freenameindex(interfaces);
    if (!index)
        errno = ENXIO;
    return index;
}

char *if_indextoname(unsigned index, char *name)
{
    if (!name)
    {
        errno = EFAULT;
        return NULL;
    }
    struct if_nameindex *interfaces = if_nameindex();
    if (!interfaces)
        return NULL;
    bool found = false;
    for (struct if_nameindex *entry = interfaces; entry->if_index; ++entry)
        if (entry->if_index == index)
        {
            size_t length = strlen(entry->if_name);
            if (length >= IF_NAMESIZE)
            {
                if_freenameindex(interfaces);
                errno = EIO;
                return NULL;
            }
            memcpy(name, entry->if_name, length + 1);
            found = true;
            break;
        }
    if_freenameindex(interfaces);
    if (!found)
    {
        errno = ENXIO;
        return NULL;
    }
    return name;
}

struct native_interface_address
{
    struct ifaddrs entry;
    struct sockaddr_in address, netmask, broadcast;
    char name[IF_NAMESIZE];
};

void freeifaddrs(struct ifaddrs *interfaces)
{
    while (interfaces)
    {
        struct ifaddrs *next = interfaces->ifa_next;
        free(interfaces);
        interfaces = next;
    }
}

int getifaddrs(struct ifaddrs **interfaces)
{
    if (!interfaces)
    {
        errno = EFAULT;
        return -1;
    }
    *interfaces = NULL;
    struct lume_net_control request = {.version = LUME_NET_CONTROL_VERSION};
    if (native_result(
            native_call(LUME_SYS_NETCTL, LUME_NET_ENUM, (uintptr_t)&request, sizeof(request))) < 0)
        return -1;
    if (request.count > LUME_NET_INTERFACES)
    {
        errno = EIO;
        return -1;
    }
    struct ifaddrs *first = NULL;
    struct ifaddrs **tail = &first;
    for (unsigned i = 0; i < request.count; ++i)
    {
        const struct lume_netif_info *source = &request.data.interfaces[i];
        size_t length = strnlen(source->name, sizeof(source->name));
        if (!source->ifindex || !length || length >= IF_NAMESIZE)
        {
            freeifaddrs(first);
            errno = EIO;
            return -1;
        }
        struct native_interface_address *address = calloc(1, sizeof(*address));
        if (!address)
        {
            int saved = errno;
            freeifaddrs(first);
            errno = saved;
            return -1;
        }
        memcpy(address->name, source->name, length + 1);
        address->entry.ifa_name = address->name;
        if (source->flags & LUME_IF_UP)
            address->entry.ifa_flags |= IFF_UP;
        if (source->flags & LUME_IF_LINK)
            address->entry.ifa_flags |= IFF_RUNNING;
        if (source->kind == LUME_IF_ETHERNET || source->kind == LUME_IF_WIFI)
            address->entry.ifa_flags |= IFF_BROADCAST;
        if (source->address_family == 4)
        {
            address->address.sin_family = AF_INET;
            address->netmask.sin_family = AF_INET;
            address->broadcast.sin_family = AF_INET;
            memcpy(&address->address.sin_addr, source->address, 4);
            memcpy(&address->netmask.sin_addr, source->netmask, 4);
            unsigned char *broadcast = (unsigned char *)&address->broadcast.sin_addr;
            for (unsigned j = 0; j < 4; ++j)
                broadcast[j] = source->address[j] | (unsigned char)~source->netmask[j];
            address->entry.ifa_addr = (struct sockaddr *)&address->address;
            address->entry.ifa_netmask = (struct sockaddr *)&address->netmask;
            if (address->entry.ifa_flags & IFF_BROADCAST)
                address->entry.ifa_broadaddr = (struct sockaddr *)&address->broadcast;
        }
        *tail = &address->entry;
        tail = &address->entry.ifa_next;
    }
    *interfaces = first;
    return 0;
}

static int socket_call(unsigned operation, struct lume_socket_request *r)
{
    r->version = LUME_SOCKET_VERSION;
    r->bytes = sizeof(*r);
    return native_result(native_call(LUME_SYS_SOCKET, operation, (uintptr_t)r, sizeof(*r)));
}

static int descriptor_flags(int type)
{
    return (type & SOCK_CLOEXEC ? LUME_FD_CLOEXEC : 0) |
           (type & SOCK_NONBLOCK ? LUME_FD_NONBLOCK : 0);
}

int socket(int family, int type, int protocol)
{
    int base_type = type & ~(SOCK_CLOEXEC | SOCK_NONBLOCK);
    if (protocol && ((base_type == SOCK_STREAM && protocol != IPPROTO_TCP) ||
                     (base_type == SOCK_DGRAM && protocol != IPPROTO_UDP) ||
                     (base_type != SOCK_STREAM && base_type != SOCK_DGRAM)))
    {
        errno = EPROTONOSUPPORT;
        return -1;
    }
    struct lume_socket_request r = {.family = family,
                                    .type = type & ~(SOCK_CLOEXEC | SOCK_NONBLOCK),
                                    .flags = descriptor_flags(type)};
    return socket_call(LUME_SOCKET_CREATE, &r) ? -1 : r.fd;
}

int socketpair(int family, int type, int protocol, int fds[2])
{
    if (protocol)
    {
        errno = EPROTONOSUPPORT;
        return -1;
    }
    struct lume_socket_request r = {.family = family,
                                    .type = type & ~(SOCK_CLOEXEC | SOCK_NONBLOCK),
                                    .flags = descriptor_flags(type)};
    if (socket_call(LUME_SOCKET_PAIR, &r))
        return -1;
    fds[0] = r.fd;
    fds[1] = r.other;
    return 0;
}

static int address_request(int fd, const struct sockaddr *address, socklen_t bytes,
                           struct lume_socket_request *r, char path[108])
{
    if (address && address->sa_family == AF_UNIX)
    {
        const struct sockaddr_un *local = (const void *)address;
        if (bytes <= offsetof(struct sockaddr_un, sun_path) || bytes > sizeof(*local))
        {
            errno = EINVAL;
            return -1;
        }
        unsigned count = (unsigned)bytes - offsetof(struct sockaddr_un, sun_path);
        size_t length = strnlen(local->sun_path, count);
        if (!length || length >= sizeof(local->sun_path))
        {
            errno = ENAMETOOLONG;
            return -1;
        }
        memcpy(path, local->sun_path, length);
        path[length] = 0;
        *r = (struct lume_socket_request){
            .fd = fd, .family = AF_UNIX, .buffer = (uintptr_t)path, .count = (unsigned)length + 1};
        return 0;
    }
    if (!address || bytes < sizeof(struct sockaddr_in) || address->sa_family != AF_INET)
    {
        errno = EAFNOSUPPORT;
        return -1;
    }
    const struct sockaddr_in *a = (const void *)address;
    *r = (struct lume_socket_request){.fd = fd, .family = AF_INET, .port = ntohs(a->sin_port)};
    memcpy(r->address, &a->sin_addr, 4);
    return 0;
}

static int return_address(const struct lume_socket_request *r, struct sockaddr *address,
                          socklen_t *bytes)
{
    if (!address)
        return 0;
    if (!bytes)
    {
        errno = EFAULT;
        return -1;
    }
    if (r->family == AF_UNIX)
    {
        struct sockaddr_un local = {.sun_family = AF_UNIX};
        size_t length = r->count < sizeof(local.sun_path) ? r->count : sizeof(local.sun_path);
        if (r->buffer && length)
            memcpy(local.sun_path, (const void *)(uintptr_t)r->buffer, length);
        socklen_t total = (socklen_t)(offsetof(struct sockaddr_un, sun_path) + length);
        memcpy(address, &local, *bytes < total ? *bytes : total);
        *bytes = total;
        return 0;
    }
    struct sockaddr_in a = {.sin_family = AF_INET, .sin_port = htons(r->port)};
    memcpy(&a.sin_addr, r->address, 4);
    memcpy(address, &a, *bytes < sizeof(a) ? *bytes : sizeof(a));
    *bytes = sizeof(a);
    return 0;
}

int bind(int fd, const struct sockaddr *address, socklen_t bytes)
{
    char path[108];
    struct lume_socket_request r;
    if (address_request(fd, address, bytes, &r, path))
        return -1;
    int result = socket_call(LUME_SOCKET_BIND, &r);
    if (result < 0 && r.family == AF_UNIX && errno == EEXIST)
        errno = EADDRINUSE;
    return result;
}

int listen(int fd, int backlog)
{
    struct lume_socket_request r = {.fd = fd, .count = backlog > 0 ? (unsigned)backlog : 1};
    return socket_call(LUME_SOCKET_LISTEN, &r);
}

static int ready(int fd, short events)
{
    struct pollfd p = {.fd = fd, .events = events};
    return poll(&p, 1, -1) < 0 ? -1 : 0;
}

int accept(int fd, struct sockaddr *address, socklen_t *bytes)
{
    struct lume_socket_request r = {.fd = fd};
    for (;;)
    {
        if (!socket_call(LUME_SOCKET_ACCEPT, &r))
        {
            if (return_address(&r, address, bytes))
            {
                close(r.other);
                return -1;
            }
            return r.other;
        }
        if (errno != EAGAIN || (fcntl(fd, F_GETFL) & O_NONBLOCK) || ready(fd, POLLIN))
            return -1;
    }
}

int connect(int fd, const struct sockaddr *address, socklen_t bytes)
{
    char path[108];
    struct lume_socket_request r;
    if (address_request(fd, address, bytes, &r, path))
        return -1;
    while (socket_call(LUME_SOCKET_CONNECT, &r))
    {
        if (r.family != AF_UNIX || errno != EAGAIN)
            return -1;
        int flags = fcntl(fd, F_GETFL);
        if (flags < 0 || (flags & O_NONBLOCK) || ready(fd, POLLOUT))
            return -1;
    }
    if (r.family == AF_UNIX || r.type == SOCK_DGRAM)
        return 0;
    if (fcntl(fd, F_GETFL) & O_NONBLOCK)
    {
        errno = EINPROGRESS;
        return -1;
    }
    if (ready(fd, POLLOUT))
        return -1;
    struct pollfd p = {.fd = fd, .events = POLLOUT};
    if (poll(&p, 1, 0) < 0)
        return -1;
    if (p.revents & (POLLERR | POLLHUP))
    {
        errno = ECONNREFUSED;
        return -1;
    }
    return 0;
}

static int socket_address(int fd, struct sockaddr *address, socklen_t *bytes, unsigned op)
{
    char path[108];
    struct lume_socket_request r = {
        .fd = fd, .family = AF_UNIX, .buffer = (uintptr_t)path, .count = sizeof(path)};
    return socket_call(op, &r) ? -1 : return_address(&r, address, bytes);
}

int getsockname(int fd, struct sockaddr *a, socklen_t *n)
{
    return socket_address(fd, a, n, LUME_SOCKET_LOCAL);
}

int getpeername(int fd, struct sockaddr *a, socklen_t *n)
{
    return socket_address(fd, a, n, LUME_SOCKET_PEER);
}

int shutdown(int fd, int direction)
{
    struct lume_socket_request r = {.fd = fd, .other = direction};
    return socket_call(LUME_SOCKET_SHUTDOWN, &r);
}

static int option_type(int level, int name)
{
    if (level == IPPROTO_TCP && name == TCP_NODELAY)
        return 1;
    if (level == SOL_SOCKET && name == SO_KEEPALIVE)
        return 2;
    if (level == SOL_SOCKET && name == SO_REUSEADDR)
        return 4;
    if (level == SOL_SOCKET && name == SO_RCVBUF)
        return LUME_SOCKET_OPTION_RECEIVE_BUFFER;
    if (level == SOL_SOCKET && name == SO_SNDBUF)
        return LUME_SOCKET_OPTION_SEND_BUFFER;
    if (level == IPPROTO_IP && name == IP_RECVTOS)
        return LUME_SOCKET_OPTION_RECEIVE_TOS;
    if (level == IPPROTO_IP && name == IP_TOS)
        return LUME_SOCKET_OPTION_TOS;
    errno = ENOPROTOOPT;
    return -1;
}

int setsockopt(int fd, int level, int name, const void *value, socklen_t bytes)
{
    int type = option_type(level, name);
    if (type < 0)
        return -1;
    if (!value || bytes != sizeof(int))
    {
        errno = EINVAL;
        return -1;
    }
    int setting;
    memcpy(&setting, value, sizeof(setting));
    bool scalar = type == LUME_SOCKET_OPTION_RECEIVE_BUFFER ||
                  type == LUME_SOCKET_OPTION_SEND_BUFFER || type == LUME_SOCKET_OPTION_TOS;
    if (scalar && (setting < 0 || (!setting && type != LUME_SOCKET_OPTION_TOS)))
    {
        errno = EINVAL;
        return -1;
    }
    struct lume_socket_request r = {
        .fd = fd, .type = type, .flags = 1, .count = scalar ? (unsigned)setting : !!setting};
    return socket_call(LUME_SOCKET_OPTION, &r);
}

int getsockopt(int fd, int level, int name, void *value, socklen_t *bytes)
{
    if (!value || !bytes || *bytes < sizeof(int))
    {
        errno = EINVAL;
        return -1;
    }
    int result;
    if (level == SOL_SOCKET && name == SO_ERROR)
    {
        struct lume_socket_request request = {.fd = fd};
        if (socket_call(LUME_SOCKET_ERROR, &request))
            return -1;
        if (request.flags & ~LUME_SOCKET_ERROR_PENDING)
        {
            errno = EIO;
            return -1;
        }
        unsigned error = request.count;
        if (!error && (request.flags & LUME_SOCKET_ERROR_PENDING))
            error = LUME_EINPROGRESS;
        result = lume_errno_from_syscall(error);
    }
    else if (level == SOL_SOCKET && name == SO_TYPE)
    {
        struct lume_socket_request request = {.fd = fd, .type = LUME_SOCKET_OPTION_TYPE};
        if (socket_call(LUME_SOCKET_OPTION, &request))
            return -1;
        result = request.count;
    }
    else
    {
        int type = option_type(level, name);
        if (type < 0)
            return -1;
        struct lume_socket_request r = {.fd = fd, .type = type};
        if (socket_call(LUME_SOCKET_OPTION, &r))
            return -1;
        result = r.count;
    }
    memcpy(value, &result, sizeof(result));
    *bytes = sizeof(result);
    return 0;
}

static int message_rights(const struct msghdr *message, struct lume_socket_message *request)
{
    if (message->msg_controllen && !message->msg_control)
    {
        errno = EFAULT;
        return -1;
    }
    const unsigned char *control = message->msg_control;
    size_t offset = 0;
    while (message->msg_controllen - offset >= sizeof(struct cmsghdr))
    {
        struct cmsghdr header;
        memcpy(&header, control + offset, sizeof(header));
        size_t remaining = message->msg_controllen - offset;
        if (header.cmsg_len < CMSG_LEN(0) || header.cmsg_len > remaining)
        {
            errno = EINVAL;
            return -1;
        }
        if (header.cmsg_level != SOL_SOCKET || header.cmsg_type != SCM_RIGHTS)
        {
            errno = EOPNOTSUPP;
            return -1;
        }
        size_t bytes = header.cmsg_len - CMSG_LEN(0);
        if (bytes % sizeof(int))
        {
            errno = EINVAL;
            return -1;
        }
        size_t count = bytes / sizeof(int);
        if (count > LUME_SOCKET_RIGHTS_MAX - request->rights_count)
        {
            errno = EMSGSIZE;
            return -1;
        }
        memcpy(request->rights + request->rights_count, control + offset + CMSG_LEN(0), bytes);
        request->rights_count += (unsigned)count;
        if (header.cmsg_len > SIZE_MAX - sizeof(size_t) + 1)
            break;
        size_t next = CMSG_ALIGN(header.cmsg_len);
        if (next > remaining)
            break;
        offset += next;
    }
    return 0;
}

static ssize_t message_io(int fd, const struct msghdr *message, struct msghdr *received, int flags)
{
    bool sending = received == NULL;
    int allowed =
        MSG_DONTWAIT | MSG_NOSIGNAL | (sending ? 0 : MSG_PEEK | MSG_CMSG_CLOEXEC | MSG_TRUNC);
    if (!message || (message->msg_iovlen && !message->msg_iov))
    {
        errno = EFAULT;
        return -1;
    }
    if ((flags & ~allowed) || message->msg_iovlen > 1024)
    {
        errno = EOPNOTSUPP;
        return -1;
    }
    struct lume_socket_message request = {.version = LUME_SOCKET_MESSAGE_VERSION,
                                          .bytes = sizeof(request)};
    unsigned rights_capacity = 0;
    size_t control_capacity = message->msg_controllen;
    if (sending)
    {
        if (message_rights(message, &request))
            return -1;
    }
    else
    {
        if (control_capacity && !message->msg_control)
        {
            errno = EFAULT;
            return -1;
        }
        if (control_capacity >= CMSG_LEN(0))
        {
            size_t capacity = (control_capacity - CMSG_LEN(0)) / sizeof(int);
            rights_capacity =
                capacity < LUME_SOCKET_RIGHTS_MAX ? (unsigned)capacity : LUME_SOCKET_RIGHTS_MAX;
        }
        request.flags = (flags & MSG_PEEK ? LUME_SOCKET_MESSAGE_PEEK : 0) |
                        (flags & MSG_CMSG_CLOEXEC ? LUME_SOCKET_MESSAGE_CLOEXEC : 0);
        received->msg_flags = 0;
        received->msg_controllen = 0;
    }
    size_t bytes = 0;
    unsigned nonempty = 0;
    void *single = NULL;
    for (size_t i = 0; i < message->msg_iovlen; ++i)
    {
        if (message->msg_iov[i].iov_len > SIZE_MAX / 2 - bytes)
        {
            errno = EINVAL;
            return -1;
        }
        if (message->msg_iov[i].iov_len)
        {
            single = message->msg_iov[i].iov_base;
            ++nonempty;
        }
        bytes += message->msg_iov[i].iov_len;
    }
    if (!bytes && request.rights_count)
    {
        errno = EINVAL;
        return -1;
    }
    if (bytes > LUME_FILE_IO_MAX)
        bytes = LUME_FILE_IO_MAX;
    unsigned char *buffer = nonempty == 1 ? single : bytes ? malloc(bytes) : NULL;
    if (bytes && !buffer)
    {
        if (nonempty == 1)
            errno = EFAULT;
        return -1;
    }
    if (sending && nonempty != 1)
    {
        size_t offset = 0;
        for (size_t i = 0; i < message->msg_iovlen && offset < bytes; ++i)
        {
            size_t length = message->msg_iov[i].iov_len;
            if (length > bytes - offset)
                length = bytes - offset;
            memcpy(buffer + offset, message->msg_iov[i].iov_base, length);
            offset += length;
        }
    }
    request.buffer = (uintptr_t)buffer;
    request.count = (unsigned)bytes;
    struct lume_socket_request call = {
        .fd = fd, .buffer = (uintptr_t)&request, .count = sizeof(request)};
    unsigned operation = sending ? LUME_SOCKET_SEND_MESSAGE : LUME_SOCKET_RECEIVE_MESSAGE;
    bool addressed = message->msg_name != NULL;
    if (addressed)
    {
        if (sending)
        {
            char path[108];
            if (request.rights_count ||
                address_request(fd, message->msg_name, message->msg_namelen, &call, path))
            {
                if (request.rights_count)
                    errno = EOPNOTSUPP;
                if (nonempty != 1)
                    free(buffer);
                return -1;
            }
            if (call.family != AF_INET || !call.port)
            {
                if (nonempty != 1)
                    free(buffer);
                errno = call.family != AF_INET ? EAFNOSUPPORT : EINVAL;
                return -1;
            }
            operation = LUME_SOCKET_SENDTO;
        }
        else
        {
            call = (struct lume_socket_request){
                .fd = fd,
                .family = AF_INET,
                .flags = flags & MSG_PEEK ? LUME_SOCKET_DATAGRAM_PEEK : 0};
            operation = LUME_SOCKET_RECVFROM;
        }
        call.buffer = (uintptr_t)buffer;
        call.count = bytes;
    }
    int result;
    for (;;)
    {
        if (!sending)
            request.rights_count = rights_capacity;
        result = socket_call(operation, &call);
        if (result >= 0 || errno != EAGAIN || (flags & MSG_DONTWAIT) ||
            (fcntl(fd, F_GETFL) & O_NONBLOCK))
            break;
        if (ready(fd, sending ? POLLOUT : POLLIN))
        {
            result = -1;
            break;
        }
    }
    int saved = errno;
    int reported = result;
    if (!sending && result >= 0)
    {
        if (addressed)
        {
            request.rights_count = 0;
            request.count = call.count;
            request.received_flags =
                call.flags & LUME_SOCKET_DATAGRAM_TRUNCATED ? LUME_SOCKET_MESSAGE_TRUNCATED : 0;
        }
        if (received->msg_name && call.family == AF_INET)
            (void)return_address(&call, received->msg_name, &received->msg_namelen);
        else
            received->msg_namelen = 0;
        if ((flags & MSG_TRUNC) && call.family == AF_INET &&
            (request.received_flags & LUME_SOCKET_MESSAGE_TRUNCATED))
            reported = request.count;

        if (nonempty != 1)
        {
            size_t offset = 0;
            for (size_t i = 0; i < message->msg_iovlen && offset < (unsigned)result; ++i)
            {
                size_t length = message->msg_iov[i].iov_len;
                if (length > (unsigned)result - offset)
                    length = (unsigned)result - offset;
                memcpy(message->msg_iov[i].iov_base, buffer + offset, length);
                offset += length;
            }
        }
        if (request.rights_count)
        {
            size_t payload = request.rights_count * sizeof(int);
            size_t used = CMSG_SPACE(payload);
            if (used > control_capacity)
                used = CMSG_LEN(payload);
            memset(received->msg_control, 0, used);
            struct cmsghdr *header = received->msg_control;
            header->cmsg_len = CMSG_LEN(payload);
            header->cmsg_level = SOL_SOCKET;
            header->cmsg_type = SCM_RIGHTS;
            memcpy(CMSG_DATA(header), request.rights, payload);
            received->msg_controllen = used;
        }
        received->msg_flags =
            (request.received_flags & LUME_SOCKET_MESSAGE_CTRUNC ? MSG_CTRUNC : 0) |
            (request.received_flags & LUME_SOCKET_MESSAGE_TRUNCATED ? MSG_TRUNC : 0);
        if (call.family == AF_INET && (call.flags & LUME_SOCKET_DATAGRAM_TOS))
        {
            if (control_capacity < CMSG_LEN(sizeof(uint8_t)))
                received->msg_flags |= MSG_CTRUNC;
            else
            {
                size_t used = CMSG_SPACE(sizeof(uint8_t));
                if (used > control_capacity)
                    used = CMSG_LEN(sizeof(uint8_t));
                memset(received->msg_control, 0, used);
                struct cmsghdr *header = received->msg_control;
                header->cmsg_len = CMSG_LEN(sizeof(uint8_t));
                header->cmsg_level = IPPROTO_IP;
                header->cmsg_type = IP_TOS;
                *(uint8_t *)CMSG_DATA(header) = (uint8_t)call.type;
                received->msg_controllen = used;
            }
        }
    }
    if (nonempty != 1)
        free(buffer);
    errno = saved;
    return reported;
}

ssize_t sendmsg(int fd, const struct msghdr *message, int flags)
{
    return message_io(fd, message, NULL, flags);
}

ssize_t recvmsg(int fd, struct msghdr *message, int flags)
{
    return message_io(fd, message, message, flags);
}

ssize_t send(int fd, const void *data, size_t bytes, int flags)
{
    struct iovec vector = {.iov_base = (void *)data, .iov_len = bytes};
    struct msghdr message = {.msg_iov = &vector, .msg_iovlen = 1};
    return sendmsg(fd, &message, flags);
}

ssize_t recv(int fd, void *data, size_t bytes, int flags)
{
    struct iovec vector = {.iov_base = data, .iov_len = bytes};
    struct msghdr message = {.msg_iov = &vector, .msg_iovlen = 1};
    return recvmsg(fd, &message, flags);
}

ssize_t sendto(int fd, const void *data, size_t bytes, int flags, const struct sockaddr *address,
               socklen_t address_bytes)
{
    struct iovec vector = {.iov_base = (void *)data, .iov_len = bytes};
    struct msghdr message = {.msg_name = (void *)address,
                             .msg_namelen = address_bytes,
                             .msg_iov = &vector,
                             .msg_iovlen = 1};
    return sendmsg(fd, &message, flags);
}

ssize_t recvfrom(int fd, void *data, size_t bytes, int flags, struct sockaddr *address,
                 socklen_t *address_bytes)
{
    if (address && !address_bytes)
    {
        errno = EFAULT;
        return -1;
    }
    struct iovec vector = {.iov_base = data, .iov_len = bytes};
    struct msghdr message = {.msg_name = address,
                             .msg_namelen = address ? *address_bytes : 0,
                             .msg_iov = &vector,
                             .msg_iovlen = 1};
    ssize_t result = recvmsg(fd, &message, flags);
    if (result >= 0 && address)
        *address_bytes = message.msg_namelen;
    return result;
}

int inet_pton(int family, const char *text, void *address)
{
    if (family != AF_INET)
    {
        errno = EAFNOSUPPORT;
        return -1;
    }
    unsigned char result[4];
    for (unsigned i = 0; i < 4; ++i)
    {
        if (*text < '0' || *text > '9')
            return 0;
        unsigned n = 0, digits = 0;
        do
        {
            n = n * 10 + *text++ - '0';
            ++digits;
        } while (*text >= '0' && *text <= '9' && digits < 4);
        if (digits > 3 || n > 255 || (i < 3 ? *text++ != '.' : *text != 0))
            return 0;
        result[i] = n;
    }
    memcpy(address, result, sizeof(result));
    return 1;
}

const char *inet_ntop(int family, const void *address, char *text, socklen_t bytes)
{
    if (family != AF_INET)
    {
        errno = EAFNOSUPPORT;
        return NULL;
    }
    const unsigned char *a = address;
    char result[INET_ADDRSTRLEN];
    int n = snprintf(result, sizeof(result), "%u.%u.%u.%u", a[0], a[1], a[2], a[3]);
    if (bytes <= (unsigned)n)
    {
        errno = ENOSPC;
        return NULL;
    }
    memcpy(text, result, n + 1);
    return text;
}

int getaddrinfo(const char *host, const char *service, const struct addrinfo *hints,
                struct addrinfo **out)
{
    *out = NULL;
    if (hints && hints->ai_family != AF_UNSPEC && hints->ai_family != AF_INET)
        return EAI_FAMILY;
    if (hints && hints->ai_socktype && hints->ai_socktype != SOCK_STREAM)
        return EAI_NONAME;
    unsigned port = 0;
    if (service)
    {
        char *end;
        unsigned long n = strtoul(service, &end, 10);
        if (!*service || *end || n > 65535)
            return EAI_NONAME;
        port = n;
    }
    struct sockaddr_in address = {.sin_family = AF_INET, .sin_port = htons(port)};
    if (!host)
        address.sin_addr.s_addr =
            hints && (hints->ai_flags & AI_PASSIVE) ? 0 : htonl(INADDR_LOOPBACK);
    else if (!strcmp(host, "localhost"))
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    else if (inet_pton(AF_INET, host, &address.sin_addr) != 1)
    {
        size_t length = strlen(host);
        struct lume_net_request request = {0};
        if ((hints && (hints->ai_flags & AI_NUMERICHOST)) || !length ||
            length >= sizeof(request.argument))
            return EAI_NONAME;
        memcpy(request.argument, host, length + 1);
        if (native_call(LUME_SYS_NET, LUME_NET_DNS, (uintptr_t)&request, sizeof(request)) < 0)
            return EAI_AGAIN;
        char *end = memchr(request.output, '\n', sizeof(request.output));
        if (!end)
            return EAI_AGAIN;
        *end = 0;
        if (inet_pton(AF_INET, request.output, &address.sin_addr) != 1)
            return EAI_AGAIN;
    }
    struct addrinfo *r = calloc(1, sizeof(*r) + sizeof(address));
    if (!r)
        return EAI_MEMORY;
    r->ai_family = AF_INET;
    r->ai_socktype = SOCK_STREAM;
    r->ai_protocol = IPPROTO_TCP;
    r->ai_addrlen = sizeof(address);
    r->ai_addr = (void *)(r + 1);
    memcpy(r->ai_addr, &address, sizeof(address));
    if (hints && (hints->ai_flags & AI_CANONNAME))
    {
        r->ai_canonname = strdup(host ? host : "0.0.0.0");
        if (!r->ai_canonname)
        {
            free(r);
            return EAI_MEMORY;
        }
    }
    *out = r;
    return 0;
}

void freeaddrinfo(struct addrinfo *address)
{
    while (address)
    {
        struct addrinfo *next = address->ai_next;
        free(address->ai_canonname);
        free(address);
        address = next;
    }
}

const char *gai_strerror(int error)
{
    return error == EAI_MEMORY ? "Address allocation failed" : "Address is unavailable";
}

int getnameinfo(const struct sockaddr *address, socklen_t bytes, char *host, socklen_t hostbytes,
                char *service, socklen_t servicebytes, int flags)
{
    if (!address || bytes < sizeof(struct sockaddr_in) || address->sa_family != AF_INET)
        return EAI_FAMILY;
    if (flags & NI_NAMEREQD)
        return EAI_NONAME;
    const struct sockaddr_in *a = (const void *)address;
    if (host && !inet_ntop(AF_INET, &a->sin_addr, host, hostbytes))
        return EAI_NONAME;
    if (service && snprintf(service, servicebytes, "%u", ntohs(a->sin_port)) >= (int)servicebytes)
        return EAI_NONAME;
    return 0;
}

struct servent *getservbyname(const char *name, const char *protocol)
{
    static struct servent service;
    if (strcmp(name, "ssh") || (protocol && strcmp(protocol, "tcp")))
        return NULL;
    service = (struct servent){.s_name = "ssh", .s_port = htons(22), .s_proto = "tcp"};
    return &service;
}

int getpeereid(int fd, uid_t *uid, gid_t *gid)
{
    if (!uid || !gid)
    {
        errno = EFAULT;
        return -1;
    }
    struct lume_socket_credentials credentials;
    struct lume_socket_request request = {
        .fd = fd, .buffer = (uintptr_t)&credentials, .count = sizeof(credentials)};
    if (socket_call(LUME_SOCKET_CREDENTIALS, &request))
        return -1;
    if (credentials.version != LUME_SOCKET_VERSION || credentials.bytes != sizeof(credentials))
    {
        errno = EIO;
        return -1;
    }
    *uid = credentials.uid;
    *gid = credentials.gid;
    return 0;
}

static _Thread_local int host_error;
static _Thread_local struct in_addr host_address;
static _Thread_local char host_name[256];
static _Thread_local char *host_addresses[2], *host_aliases[1];
static _Thread_local struct hostent host_entry;

int *__lume_h_errno(void)
{
    return &host_error;
}

char *inet_ntoa(struct in_addr address)
{
    static _Thread_local char text[INET_ADDRSTRLEN];
    return inet_ntop(AF_INET, &address, text, sizeof(text)) ? text : NULL;
}

in_addr_t inet_addr(const char *text)
{
    struct in_addr address;
    return text && inet_pton(AF_INET, text, &address) == 1 ? address.s_addr : INADDR_NONE;
}

struct hostent *gethostbyname(const char *name)
{
    if (!name || strlen(name) >= sizeof(host_name))
    {
        host_error = HOST_NOT_FOUND;
        return NULL;
    }
    if (!strcmp(name, "localhost") || !strcmp(name, "lume"))
        host_address.s_addr = htonl(INADDR_LOOPBACK);
    else if (inet_pton(AF_INET, name, &host_address) != 1)
    {
        struct addrinfo *address = NULL;
        int result = getaddrinfo(name, NULL, NULL, &address);
        if (result)
        {
            host_error = result == EAI_AGAIN ? TRY_AGAIN : HOST_NOT_FOUND;
            return NULL;
        }
        host_address = ((struct sockaddr_in *)address->ai_addr)->sin_addr;
        freeaddrinfo(address);
    }
    strcpy(host_name, name);
    host_addresses[0] = (char *)&host_address;
    host_addresses[1] = NULL;
    host_aliases[0] = NULL;
    host_entry = (struct hostent){.h_name = host_name,
                                  .h_aliases = host_aliases,
                                  .h_addrtype = AF_INET,
                                  .h_length = sizeof(host_address),
                                  .h_addr_list = host_addresses};
    host_error = 0;
    return &host_entry;
}

struct hostent *gethostbyaddr(const void *address, socklen_t bytes, int family)
{
    if (family != AF_INET || bytes != sizeof(struct in_addr))
    {
        host_error = HOST_NOT_FOUND;
        return NULL;
    }
    char name[INET_ADDRSTRLEN];
    if (!inet_ntop(AF_INET, address, name, sizeof(name)))
        return NULL;
    return gethostbyname(name);
}
