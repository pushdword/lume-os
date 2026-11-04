/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_DILLO_LOCAL_SOCKET_H
#define LUME_DILLO_LOCAL_SOCKET_H
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

static inline int lume_dpi_address(const struct sockaddr *address, socklen_t bytes,
                                   struct sockaddr_un *local)
{
    if (!address || bytes < sizeof(struct sockaddr_in) || address->sa_family != AF_INET)
    {
        errno = EAFNOSUPPORT;
        return -1;
    }
    const struct sockaddr_in *peer = (const void *)address;
    const char *home = getenv("HOME");
    if (!home || !*home || peer->sin_addr.s_addr != inet_addr("127.0.0.1") || !peer->sin_port)
    {
        errno = EINVAL;
        return -1;
    }
    memset(local, 0, sizeof(*local));
    local->sun_family = AF_UNIX;
    int length = snprintf(local->sun_path, sizeof(local->sun_path), "%s/.dillo/dpi-%u", home,
                          (unsigned)ntohs(peer->sin_port));
    if (length < 0 || (size_t)length >= sizeof(local->sun_path))
    {
        errno = ENAMETOOLONG;
        return -1;
    }
    return 0;
}

static inline int lume_dpi_socket(int family, int type, int protocol)
{
    if (family != AF_INET || protocol)
    {
        errno = EAFNOSUPPORT;
        return -1;
    }
    return socket(AF_UNIX, type, 0);
}

static inline int lume_dpi_connect(int fd, const struct sockaddr *address, socklen_t bytes)
{
    struct sockaddr_un local;
    return lume_dpi_address(address, bytes, &local)
               ? -1
               : connect(fd, (const struct sockaddr *)&local, sizeof(local));
}

static inline int lume_dpi_bind(int fd, const struct sockaddr *address, socklen_t bytes)
{
    struct sockaddr_un local;
    return lume_dpi_address(address, bytes, &local)
               ? -1
               : bind(fd, (const struct sockaddr *)&local, sizeof(local));
}

#define socket lume_dpi_socket
#define connect lume_dpi_connect
#define bind lume_dpi_bind
#endif
