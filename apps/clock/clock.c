/* SPDX-License-Identifier: BSD-3-Clause */
#include "sntp.h"
#include "lume/syscall_native.h"
#include "lume/abi.h"
#include "lume/io.h"
#include "lume/realtime_abi.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static int stream(unsigned operation, struct lume_stream_request *request)
{
    return (int)lume_native_call(LUME_SYS_STREAM, operation, (uintptr_t)request, sizeof(*request));
}

static int synchronize(const char *server, unsigned port, struct lume_realtime_request *clock)
{
    struct addrinfo *address = NULL;
    if (getaddrinfo(server, NULL, NULL, &address))
    {
        fprintf(stderr, "clock: cannot resolve %s\n", server);
        return 1;
    }
    struct lume_stream_request request = {.version = LUME_IO_VERSION, .port = port};
    memcpy(request.address, &((struct sockaddr_in *)address->ai_addr)->sin_addr, 4);
    freeaddrinfo(address);
    if (stream(LUME_DATAGRAM_CONNECT, &request) < 0)
    {
        fputs("clock: cannot open time service\n", stderr);
        return 1;
    }
    unsigned char nonce[8];
    int result = 1;
    if (lume_native_call(LUME_SYS_RANDOM, (uintptr_t)nonce, sizeof(nonce), 0) != sizeof(nonce))
        goto finished;
    memset(request.data, 0, 48);
    request.data[0] = 0x23;
    memcpy(request.data + 40, nonce, sizeof(nonce));
    request.count = 48;
    uint64_t sent = (uint64_t)lume_native_call(LUME_SYS_CLOCK, 0, 0, 0);
    if (stream(LUME_STREAM_SEND, &request) < 0)
        goto finished;
    while ((uint64_t)lume_native_call(LUME_SYS_CLOCK, 0, 0, 0) - sent < 5000)
    {
        if (stream(LUME_STREAM_POLL, &request) < 0)
            break;
        unsigned events = request.count;
        if (request.count & LUME_STREAM_READABLE)
        {
            request.count = sizeof(request.data);
            if (stream(LUME_STREAM_RECEIVE, &request) < 0)
                break;
            if (sntp_reply(request.data, request.count, nonce, &clock->seconds,
                           &clock->nanoseconds))
            {
                int64_t set = lume_native_call(LUME_SYS_REALTIME, LUME_REALTIME_SET,
                                               (uintptr_t)clock, sizeof(*clock));
                if (set < 0)
                    fprintf(stderr, "clock: cannot set UTC (%lld)\n", (long long)set);
                else
                    result = 0;
                break;
            }
        }
        if (events & (LUME_STREAM_ERROR | LUME_STREAM_HANGUP))
            break;
        (void)lume_native_call(LUME_SYS_SLEEP, 20, 0, 0);
    }
finished:
    (void)stream(LUME_STREAM_CLOSE, &request);
    if (result)
        fputs("clock: synchronization failed\n", stderr);
    return result;
}

int main(int argc, char **argv)
{
    struct lume_realtime_request request = {.version = LUME_REALTIME_VERSION,
                                            .bytes = sizeof(request)};
    if (argc >= 2 && !strcmp(argv[1], "sync") && argc <= 4)
    {
        char *end = NULL;
        unsigned long port = argc == 4 ? strtoul(argv[3], &end, 10) : 123;
        if (!port || port > 65535 || (end && (!*argv[3] || *end || argv[3][0] == '-')))
        {
            fputs("clock: invalid time service port\n", stderr);
            return 2;
        }
        if (synchronize(argc >= 3 ? argv[2] : "time.cloudflare.com", (unsigned)port, &request))
            return 1;
    }
    else if (argc == 3 && !strcmp(argv[1], "set"))
    {
        char *end;
        request.seconds = strtoull(argv[2], &end, 10);
        if (!*argv[2] || *end || argv[2][0] == '-' ||
            lume_native_call(LUME_SYS_REALTIME, LUME_REALTIME_SET, (uintptr_t)&request,
                             sizeof(request)) < 0)
        {
            fputs("clock: invalid epoch or clock administration unavailable\n", stderr);
            return 1;
        }
    }
    else if (argc != 1)
    {
        fputs("Usage: clock [sync [server [port]] | set unix-seconds]\n", stderr);
        return 2;
    }
    request.seconds = 0;
    request.nanoseconds = 0;
    if (lume_native_call(LUME_SYS_REALTIME, LUME_REALTIME_GET, (uintptr_t)&request,
                         sizeof(request)) < 0)
    {
        fputs("clock: UTC is unset. Run clock sync.\n", stderr);
        return 1;
    }
    time_t seconds = (time_t)request.seconds;
    struct tm date;
    char text[40];
    if (!gmtime_r(&seconds, &date) || !strftime(text, sizeof(text), "%Y-%m-%d %H:%M:%S UTC", &date))
        return 1;
    puts(text);
    return 0;
}
