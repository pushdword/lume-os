/* SPDX-License-Identifier: BSD-3-Clause */
#include "lume/abi.h"
#include "lume/syscall_native.h"
#include "perf.h"
#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

static uint32_t table[256];

static uint64_t milliseconds(void)
{
    struct timespec value;
    if (clock_gettime(CLOCK_MONOTONIC, &value))
        return 0;
    return (uint64_t)value.tv_sec * 1000 + (unsigned)value.tv_nsec / 1000000;
}

int main(int argc, char **argv)
{
    if (argc != 5 || (strcmp(argv[1], "rx") && strcmp(argv[1], "tx")))
        return 2;
    int transmit = !strcmp(argv[1], "tx");
    unsigned port = (unsigned)strtoul(argv[3], NULL, 10);
    unsigned total = (unsigned)strtoul(argv[4], NULL, 10);
    if (!port || port > 65535 || !total || total > 16 * 1024 * 1024)
        return 2;
    for (unsigned i = 0; i < 256; ++i)
    {
        uint32_t value = i;
        for (unsigned bit = 0; bit < 8; ++bit)
            value = (value >> 1) ^ (value & 1 ? UINT32_C(0xedb88320) : 0);
        table[i] = value;
    }
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0)
        return 1;
    struct sockaddr_in address = {.sin_family = AF_INET, .sin_port = htons(port)};
    if (inet_pton(AF_INET, argv[2], &address.sin_addr) != 1 ||
        bind(listener, (struct sockaddr *)&address, sizeof(address)) || listen(listener, 1))
        return 1;
    printf("NET_PERF_READY address=%s port=%u mode=%s bytes=%u\n", argv[2], port, argv[1], total);
    fflush(stdout);
    struct pollfd waiting = {.fd = listener, .events = POLLIN};
    if (poll(&waiting, 1, 15000) <= 0)
        return 1;
    int peer = accept(listener, NULL, NULL);
    close(listener);
    if (peer < 0)
        return 1;
    struct lume_perf_snapshot before, after;
    if (lume_native_call(LUME_SYS_TELEMETRY, (uintptr_t)&before, sizeof(before), LUME_PERF_VERSION))
        return 1;
    unsigned char data[4096];
    unsigned done = 0;
    uint32_t crc = UINT32_MAX;
    uint64_t start = milliseconds();
    while (done < total && milliseconds() - start < 120000)
    {
        unsigned count = total - done < sizeof(data) ? total - done : sizeof(data);
        if (transmit)
            for (unsigned i = 0; i < count; ++i)
                data[i] = (unsigned char)((done + i) * 13u + 7u);
        waiting = (struct pollfd){.fd = peer, .events = transmit ? POLLOUT : POLLIN};
        if (poll(&waiting, 1, 10000) <= 0)
            break;
        ssize_t bytes = transmit ? write(peer, data, count) : read(peer, data, count);
        if (bytes < 0 && (errno == EINTR || errno == EAGAIN))
            continue;
        if (bytes <= 0 || (unsigned)bytes > count)
            break;
        for (ssize_t i = 0; i < bytes; ++i)
            crc = table[(crc ^ data[i]) & 255u] ^ (crc >> 8);
        done += (unsigned)bytes;
    }
    uint64_t elapsed = milliseconds() - start;
    if (lume_native_call(LUME_SYS_TELEMETRY, (uintptr_t)&after, sizeof(after), LUME_PERF_VERSION))
        return 1;
    if (!transmit && done == total)
    {
        uint32_t checksum = ~crc;
        if (write(peer, &checksum, sizeof(checksum)) != sizeof(checksum))
            return 1;
    }
    close(peer);
    printf("NET_PERF mode=%s bytes=%u crc32=%08x elapsed_ms=%llu cores=%u\n", argv[1], done, ~crc,
           (unsigned long long)elapsed, after.core_count);
    for (unsigned core = 0; core < after.core_count; ++core)
    {
        printf("NET_PERF_CORE index=%u counter=%u", core,
               after.cores[core].counter - before.cores[core].counter);
        for (unsigned kind = 0; kind < LUME_PERF_MODES; ++kind)
            printf(" ticks%u=%u", kind,
                   after.cores[core].ticks[kind] - before.cores[core].ticks[kind]);
        puts("");
    }
    if (done != total)
        return 1;
    puts("NET_PERF_PASS");
    return 0;
}
