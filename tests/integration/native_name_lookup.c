/* SPDX-License-Identifier: BSD-3-Clause */
#include "lume/abi.h"
#include "lume/net_abi.h"
#include "lume/net_commands.h"
#include "lume/syscall_native.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    if (argc != 2 || strlen(argv[1]) >= sizeof(((struct lume_net_request *)0)->argument))
        return 2;
    struct lume_net_request request = {0};
    memcpy(request.argument, argv[1], strlen(argv[1]) + 1);
    int64_t result =
        lume_native_call(LUME_SYS_NET, LUME_NET_DNS, (uintptr_t)&request, sizeof(request));
    printf("NATIVE_DNS result=%lld output=%.100s\n", (long long)result, request.output);
    struct addrinfo hints = {.ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM};
    struct addrinfo *addresses = NULL;
    int error = getaddrinfo(argv[1], NULL, &hints, &addresses);
    printf("POSIX_DNS result=%d errno=%d family=%d bytes=%u\n", error, errno,
           addresses ? addresses->ai_family : -1, addresses ? addresses->ai_addrlen : 0);
    for (struct addrinfo *item = addresses; item; item = item->ai_next)
    {
        char address[INET_ADDRSTRLEN];
        if (item->ai_family == AF_INET &&
            inet_ntop(AF_INET, &((struct sockaddr_in *)item->ai_addr)->sin_addr, address,
                      sizeof(address)))
            printf("POSIX_DNS address=%s\n", address);
    }
    freeaddrinfo(addresses);
    return error != 0 || result < 0;
}
