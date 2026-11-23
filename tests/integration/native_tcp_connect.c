/* SPDX-License-Identifier: BSD-3-Clause */
#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc != 3)
        return 64;
    char *end = NULL;
    long port = strtol(argv[2], &end, 10);
    if (!end || *end || port < 1 || port > 65535)
        return 64;

    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0)
        return 1;
    int initial_flags = fcntl(fd, F_GETFL);
    int set_result = initial_flags < 0 ? -1 : fcntl(fd, F_SETFL, initial_flags | O_NONBLOCK);
    int nonblocking_flags = fcntl(fd, F_GETFL);
    printf("TCP_FLAGS initial=%d set=%d nonblocking=%d errno=%d\n", initial_flags, set_result,
           nonblocking_flags, errno);
    if (initial_flags < 0 || set_result < 0 || nonblocking_flags < 0 ||
        !(nonblocking_flags & O_NONBLOCK))
    {
        close(fd);
        return 2;
    }

    struct sockaddr_in peer = {.sin_family = AF_INET, .sin_port = htons((unsigned short)port)};
    if (inet_pton(AF_INET, argv[1], &peer.sin_addr) != 1)
    {
        close(fd);
        return 64;
    }
    errno = 0;
    int connect_result = connect(fd, (const struct sockaddr *)&peer, sizeof(peer));
    int connect_error = errno;
    int connected_flags = fcntl(fd, F_GETFL);
    int flags_error = errno;
    printf("TCP_CONNECT result=%d errno=%d flags=%d flags_errno=%d\n", connect_result,
           connect_error, connected_flags, flags_error);
    if (connect_result < 0 && connect_error != EINPROGRESS)
    {
        close(fd);
        return 3;
    }

    struct pollfd event = {.fd = fd, .events = POLLOUT};
    int poll_result = poll(&event, 1, 5000);
    int poll_error = errno;
    int socket_error = -1;
    socklen_t error_bytes = sizeof(socket_error);
    int option_result = getsockopt(fd, SOL_SOCKET, SO_ERROR, &socket_error, &error_bytes);
    printf("TCP_READY poll=%d revents=%u errno=%d option=%d socket_error=%d bytes=%u\n",
           poll_result, (unsigned)event.revents, poll_error, option_result, socket_error,
           (unsigned)error_bytes);
    if (poll_result != 1 || !(event.revents & POLLOUT) || option_result || socket_error)
    {
        close(fd);
        return 4;
    }
    if (fcntl(fd, F_SETFL, connected_flags & ~O_NONBLOCK))
    {
        close(fd);
        return 5;
    }

    static const char request[] = "GET /payload HTTP/1.0\r\nHost: 10.0.2.100\r\n\r\n";
    ssize_t sent = send(fd, request, sizeof(request) - 1, 0);
    struct pollfd incoming = {.fd = fd, .events = POLLIN};
    errno = 0;
    int incoming_result = poll(&incoming, 1, 5000);
    int incoming_error = errno;
    printf("TCP_INCOMING poll=%d revents=%u errno=%d\n", incoming_result,
           (unsigned)incoming.revents, incoming_error);
    if (incoming_result != 1 || !(incoming.revents & POLLIN))
    {
        close(fd);
        return 6;
    }
    char response[512];
    ssize_t received = recv(fd, response, sizeof(response) - 1, 0);
    printf("TCP_IO sent=%ld received=%ld errno=%d\n", (long)sent, (long)received, errno);
    close(fd);
    if (received > 0)
        response[received] = 0;
    if (sent != (ssize_t)(sizeof(request) - 1) || received <= 0 || !strstr(response, "200 OK"))
        return 7;
    puts("NATIVE_TCP_CONNECT_PASS");
    return 0;
}
