#include "lume/abi.h"
#include "lume/socket.h"
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static int64_t native(unsigned number, uint64_t a, uint64_t b, uint64_t c)
{
    register uint64_t x8 __asm__("x8") = number;
    register uint64_t x0 __asm__("x0") = a, x1 __asm__("x1") = b, x2 __asm__("x2") = c;
    __asm__ volatile("svc #0x4c55" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2) : "memory", "cc");
    return (int64_t)x0;
}

static void require(int value, const char *message)
{
    if (!value)
    {
        fprintf(stderr, "SOCKET_PROBE_FAIL %s\n", message);
        exit(1);
    }
}

static int call(unsigned operation, struct lume_socket_request *request)
{
    request->version = LUME_SOCKET_VERSION;
    request->bytes = sizeof(*request);
    return (int)native(LUME_SYS_SOCKET, operation, (uintptr_t)request, sizeof(*request));
}

int main(int argc, char **argv)
{
    require(argc == 2 && !strncmp(argv[1], "http://10.0.2.2:", 16), "HTTP endpoint");
    struct lume_socket_request pair = {.family = 1, .type = 1};
    require(!call(LUME_SOCKET_PAIR, &pair), "local socket pair");
    int first = pair.fd, second = pair.other;
    int file = open("/data/socket-probe.tmp", O_CREAT | O_TRUNC | O_RDWR, 0600);
    require(file >= 0 && write(file, "secret", 6) == 6 && !lseek(file, 0, SEEK_SET), "file setup");
    require(write(first, "hello", 5) == 5, "local write");
    struct pollfd ready = {second, POLLIN, 0};
    require(poll(&ready, 1, 0) == 1 && ready.revents & POLLIN, "local readiness");
    int64_t pid = native(LUME_SYS_FORK, 0, 0, 0);
    require(pid >= 0, "socket inheritance fork");
    if (!pid)
    {
        char data[2];
        require(read(second, data, 2) == 2 && !memcmp(data, "he", 2), "inherited receive queue");
        char byte = 'R';
        struct lume_socket_request send = {
            .fd = first, .other = file, .buffer = (uintptr_t)&byte, .count = 1};
        require(call(LUME_SOCKET_SEND_FD, &send) == 1, "send descriptor");
        require(!close(file), "sender descriptor close");
        return 19;
    }
    int status;
    require(waitpid((pid_t)pid, &status, 0) == pid && WEXITSTATUS(status) == 19, "sender exit");
    require(!close(file), "last original descriptor close");
    char rest[3], byte;
    require(read(second, rest, 3) == 3 && !memcmp(rest, "llo", 3), "shared receive offset");
    struct lume_socket_request receive = {.fd = second, .buffer = (uintptr_t)&byte, .count = 1};
    require(call(LUME_SOCKET_RECEIVE_FD, &receive) == 1 && byte == 'R' && receive.other >= 0,
            "receive descriptor after sender exit");
    char contents[8] = {0};
    require(read(receive.other, contents, 6) == 6 && !strcmp(contents, "secret"),
            "received file contents");
    require(!close(receive.other), "received descriptor close");
    struct lume_socket_request cycle = {
        .fd = first, .other = second, .buffer = (uintptr_t)&byte, .count = 1};
    require(call(LUME_SOCKET_SEND_FD, &cycle) == -LUME_ENOSYS, "local descriptor cycle rejection");
    require(!close(first) && read(second, rest, 1) == 0 && !close(second), "local EOF and release");
    for (unsigned i = 0; i < 32; ++i)
    {
        pair = (struct lume_socket_request){.family = 1, .type = 1};
        require(!call(LUME_SOCKET_PAIR, &pair), "pair reuse");
        require(!close(pair.fd) && !close(pair.other), "pair reuse release");
    }
    require(!unlink("/data/socket-probe.tmp"), "file removal");
    struct lume_socket_request tcp = {.family = 2, .type = 1};
    require(!call(LUME_SOCKET_CREATE, &tcp), "TCP descriptor create");
    tcp.address[0] = 10;
    tcp.address[1] = 0;
    tcp.address[2] = 2;
    tcp.address[3] = 2;
    tcp.port = (unsigned)atoi(argv[1] + 16);
    require(!call(LUME_SOCKET_CONNECT, &tcp), "TCP connect");
    ready = (struct pollfd){tcp.fd, POLLOUT, 0};
    require(poll(&ready, 1, 10000) == 1 && ready.revents & POLLOUT, "TCP connect readiness");
    const char request[] = "GET /payload HTTP/1.0\r\nHost: 10.0.2.2\r\n\r\n";
    require(write(tcp.fd, request, sizeof(request) - 1) == sizeof(request) - 1,
            "TCP descriptor write");
    char response[1024];
    unsigned used = 0;
    for (;;)
    {
        ready = (struct pollfd){tcp.fd, POLLIN, 0};
        require(poll(&ready, 1, 10000) == 1, "TCP receive readiness");
        ssize_t count = read(tcp.fd, response + used, sizeof(response) - 1 - used);
        require(count >= 0, "TCP descriptor read");
        if (!count)
            break;
        used += (unsigned)count;
        require(used + 1 < sizeof(response), "TCP response bound");
    }
    response[used] = 0;
    require(strstr(response, "200 OK") && strstr(response, "SOCKET_TCP_PAYLOAD"), "TCP payload");
    require(!close(tcp.fd), "TCP descriptor release");
    puts("SOCKET_FORK_RIGHTS_TCP_PASS");
    return 0;
}
