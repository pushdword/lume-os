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
#include <net/if.h>
#include "lume/net_abi.h"

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
    if (protocol && protocol != IPPROTO_TCP)
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
    return address_request(fd, address, bytes, &r, path) ? -1 : socket_call(LUME_SOCKET_BIND, &r);
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
    if (r.family == AF_UNIX)
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
    struct lume_socket_request r = {
        .fd = fd, .type = type, .flags = 1, .count = !!*(const int *)value};
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
    if (level == SOL_SOCKET && (name == SO_ERROR || name == SO_TYPE))
    {
        struct stat metadata;
        if (fstat(fd, &metadata))
            return -1;
        if (!S_ISSOCK(metadata.st_mode))
        {
            errno = ENOTSOCK;
            return -1;
        }
        struct pollfd p = {.fd = fd, .events = POLLOUT};
        if (poll(&p, 1, 0) < 0)
            return -1;
        result = name == SO_TYPE ? SOCK_STREAM : p.revents & POLLERR ? ECONNREFUSED : 0;
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

ssize_t send(int fd, const void *data, size_t bytes, int flags)
{
    if (flags & ~MSG_NOSIGNAL)
    {
        errno = EOPNOTSUPP;
        return -1;
    }
    return write(fd, data, bytes);
}

ssize_t recv(int fd, void *data, size_t bytes, int flags)
{
    if (flags)
    {
        errno = EOPNOTSUPP;
        return -1;
    }
    return read(fd, data, bytes);
}

ssize_t sendmsg(int fd, const struct msghdr *message, int flags)
{
    if ((flags & ~MSG_NOSIGNAL) || message->msg_name || !message->msg_iov ||
        message->msg_iovlen > 1024)
    {
        errno = EOPNOTSUPP;
        return -1;
    }
    if (!message->msg_controllen)
        return writev(fd, message->msg_iov, (int)message->msg_iovlen);
    const struct cmsghdr *c = CMSG_FIRSTHDR(message);
    if (!c || c->cmsg_level != SOL_SOCKET || c->cmsg_type != SCM_RIGHTS ||
        c->cmsg_len != CMSG_LEN(sizeof(int)) || message->msg_controllen > CMSG_SPACE(sizeof(int)) ||
        message->msg_iovlen != 1 || message->msg_iov[0].iov_len != 1)
    {
        errno = EOPNOTSUPP;
        return -1;
    }
    int passed;
    memcpy(&passed, CMSG_DATA(c), sizeof(passed));
    struct lume_socket_request r = {
        .fd = fd, .other = passed, .count = 1, .buffer = (uintptr_t)message->msg_iov[0].iov_base};
    int result;
    do
    {
        result = socket_call(LUME_SOCKET_SEND_FD, &r);
        if (result >= 0 || errno != EAGAIN || (fcntl(fd, F_GETFL) & O_NONBLOCK))
            return result;
    } while (!ready(fd, POLLOUT));
    return -1;
}

ssize_t recvmsg(int fd, struct msghdr *message, int flags)
{
    if (flags || message->msg_name || message->msg_iovlen != 1 || !message->msg_iov)
    {
        errno = EOPNOTSUPP;
        return -1;
    }
    struct lume_socket_request r = {
        .fd = fd,
        .count = message->msg_iov[0].iov_len > 65536 ? 65536 : message->msg_iov[0].iov_len,
        .buffer = (uintptr_t)message->msg_iov[0].iov_base};
    int result;
    do
    {
        result = socket_call(LUME_SOCKET_RECEIVE_FD, &r);
        if (result >= 0 || errno != EAGAIN || (fcntl(fd, F_GETFL) & O_NONBLOCK))
            break;
        if (ready(fd, POLLIN))
            return -1;
    } while (1);
    if (result < 0)
        return -1;
    message->msg_flags = 0;
    size_t capacity = message->msg_controllen;
    message->msg_controllen = 0;
    if (r.other >= 0)
    {
        if (message->msg_control && capacity >= CMSG_SPACE(sizeof(int)))
        {
            struct cmsghdr *c = message->msg_control;
            memset(c, 0, CMSG_SPACE(sizeof(int)));
            c->cmsg_level = SOL_SOCKET;
            c->cmsg_type = SCM_RIGHTS;
            c->cmsg_len = CMSG_LEN(sizeof(int));
            memcpy(CMSG_DATA(c), &r.other, sizeof(int));
            message->msg_controllen = CMSG_SPACE(sizeof(int));
        }
        else
        {
            close(r.other);
            message->msg_flags |= MSG_CTRUNC;
        }
    }
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
        return EAI_NONAME;
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
    (void)fd;
    (void)uid;
    (void)gid;
    return unsupported();
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
        host_error = HOST_NOT_FOUND;
        return NULL;
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
