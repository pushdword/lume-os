/* SPDX-License-Identifier: BSD-3-Clause */
#include "lume/abi.h"
#include "lume/net_abi.h"
#include "lume/net_commands.h"
#include "lume/syscall_native.h"
#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

static const struct addrinfo stream_hints = {
    .ai_flags = AI_CANONNAME, .ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM};

static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t peers = PTHREAD_COND_INITIALIZER;
static unsigned ready;
static int run_lookups;

struct lookup
{
    const char *host;
    struct addrinfo *result;
    int error;
};

static void *resolve(void *argument)
{
    struct lookup *lookup = argument;
    assert(!pthread_mutex_lock(&gate));
    ++ready;
    assert(!pthread_cond_broadcast(&peers));
    while (!run_lookups)
        assert(!pthread_cond_wait(&peers, &gate));
    assert(!pthread_mutex_unlock(&gate));
    lookup->error = getaddrinfo(lookup->host, "443", &stream_hints, &lookup->result);
    return NULL;
}

static struct in_addr ipv4_answer(const struct addrinfo *result, unsigned port)
{
    assert(result && result->ai_family == AF_INET && result->ai_socktype == SOCK_STREAM &&
           result->ai_protocol == IPPROTO_TCP && result->ai_addrlen == sizeof(struct sockaddr_in));
    const struct sockaddr_in *address = (const void *)result->ai_addr;
    assert(address && address->sin_family == AF_INET && ntohs(address->sin_port) == port);
    return address->sin_addr;
}

static void check_numeric_calls(void)
{
    struct addrinfo hints = stream_hints;
    hints.ai_flags = AI_NUMERICHOST;
    struct addrinfo *address = NULL;
    assert(getaddrinfo("localhost", "443", &hints, &address) == EAI_NONAME && !address);
    assert(!getaddrinfo("127.0.0.1", "443", &hints, &address));
    assert(ipv4_answer(address, 443).s_addr == htonl(INADDR_LOOPBACK));
    freeaddrinfo(address);
    hints.ai_flags = AI_PASSIVE;
    assert(!getaddrinfo(NULL, "443", &hints, &address));
    assert(ipv4_answer(address, 443).s_addr == htonl(INADDR_ANY));
    freeaddrinfo(address);
    assert(!getaddrinfo("localhost", "443", &hints, &address));
    assert(ipv4_answer(address, 443).s_addr == htonl(INADDR_LOOPBACK));
    freeaddrinfo(address);
}

static struct in_addr check_parallel_lookups(const char *host)
{
    struct lookup requests[2] = {{.host = host}, {.host = host}};
    pthread_t workers[2];
    for (unsigned i = 0; i < 2; ++i)
        assert(!pthread_create(&workers[i], NULL, resolve, &requests[i]));
    assert(!pthread_mutex_lock(&gate));
    while (ready < 2)
        assert(!pthread_cond_wait(&peers, &gate));
    run_lookups = 1;
    assert(!pthread_cond_broadcast(&peers));
    assert(!pthread_mutex_unlock(&gate));
    for (unsigned i = 0; i < 2; ++i)
    {
        assert(!pthread_join(workers[i], NULL));
        if (requests[i].error)
            printf("POSIX_DNS_WORKER_ERROR worker=%u result=%d\n", i, requests[i].error);
        assert(!requests[i].error && requests[i].result);
    }
    const struct addrinfo *first = requests[0].result, *second = requests[1].result;
    assert(first != second && first->ai_addr != second->ai_addr);
    assert(first->ai_canonname && second->ai_canonname && first->ai_canonname != second->ai_canonname);
    assert(!strcmp(first->ai_canonname, host));
    struct in_addr address = ipv4_answer(first, 443);
    freeaddrinfo(requests[0].result);
    assert(!strcmp(second->ai_canonname, host) && ipv4_answer(second, 443).s_addr == address.s_addr);
    freeaddrinfo(requests[1].result);
    return address;
}

static int64_t native_lookup(const char *host, struct lume_net_request *request)
{
    size_t length = strlen(host);
    assert(length < sizeof(request->argument));
    *request = (struct lume_net_request){0};
    memcpy(request->argument, host, length + 1);
    return lume_native_call(LUME_SYS_NET, LUME_NET_DNS, (uintptr_t)request, sizeof(*request));
}

static void check_failure(const char *host, int native_error, int posix_error, int host_error,
                          const char *label)
{
    struct lume_net_request request;
    struct addrinfo *address = NULL;
    assert(native_lookup(host, &request) == -native_error && !request.output[0]);
    int result = getaddrinfo(host, NULL, &stream_hints, &address);
    assert(result == posix_error && !address);
    assert(!gethostbyname(host) && h_errno == host_error);
    printf("%s %d\n", label, result);
}

int main(int argc, char **argv)
{
    if (argc < 2 || argc > 4 || strlen(argv[1]) >= sizeof(((struct lume_net_request *)0)->argument))
        return 2;
    check_numeric_calls();
    struct in_addr address = check_parallel_lookups(argv[1]);
    struct lume_net_request request;
    int64_t result = native_lookup(argv[1], &request);
    char text[INET_ADDRSTRLEN];
    assert(result >= 0 && inet_ntop(AF_INET, &address, text, sizeof(text)));
    size_t length = strlen(text);
    assert(!memcmp(request.output, text, length) && request.output[length] == '\n');
    printf("NATIVE_DNS_IPV4 %s\n", text);
    if (argc >= 3)
        check_failure(argv[2], LUME_ENOENT, EAI_NONAME, HOST_NOT_FOUND, "NATIVE_DNS_NEGATIVE");
    if (argc == 4)
        check_failure(argv[3], LUME_EIO, EAI_AGAIN, TRY_AGAIN, "NATIVE_DNS_TEMPORARY");
    puts("NATIVE_NAME_LOOKUP_PASS");
    return 0;
}
