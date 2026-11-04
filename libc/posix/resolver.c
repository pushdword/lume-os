/* SPDX-License-Identifier: BSD-3-Clause */
#include "native_posix.h"
#include "lume/io.h"
#include "lume/net_abi.h"
#include <netdb.h>
#include <resolv.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static int query_name(const char *name, unsigned char query[272], int qclass, int type)
{
    if (!name || qclass < 0 || qclass > 65535 || type < 0 || type > 65535)
        return -1;
    size_t length = strnlen(name, 255);
    if (length && name[length - 1] == '.')
        --length;
    if (length > 253 || (length && name[length - 1] == '.'))
        return -1;
    memset(query, 0, 272);
    if (native_call(LUME_SYS_RANDOM, (uintptr_t)query, 2, 0) != 2)
        return -1;
    query[2] = 1;
    query[5] = 1;
    unsigned position = 12;
    for (size_t first = 0; first < length;)
    {
        size_t last = first;
        while (last < length && name[last] != '.')
            ++last;
        size_t count = last - first;
        if (!count || count > 63)
            return -1;
        query[position++] = (unsigned char)count;
        memcpy(query + position, name + first, count);
        position += (unsigned)count;
        first = last + 1;
    }
    query[position++] = 0;
    ns_put16((unsigned)type, query + position);
    ns_put16((unsigned)qclass, query + position + 2);
    return (int)position + 4;
}

static int dns_server(unsigned char address[4])
{
    struct lume_net_control request = {.version = LUME_NET_CONTROL_VERSION};
    if (native_result(
            native_call(LUME_SYS_NETCTL, LUME_NET_ENUM, (uintptr_t)&request, sizeof(request))) < 0)
        return -1;
    if (request.count > LUME_NET_INTERFACES)
        return -1;
    for (unsigned i = 0; i < request.count; ++i)
    {
        const struct lume_netif_info *interface = &request.data.interfaces[i];
        if ((interface->flags & (LUME_IF_UP | LUME_IF_LINK)) == (LUME_IF_UP | LUME_IF_LINK) &&
            (interface->dns[0] || interface->dns[1] || interface->dns[2] || interface->dns[3]))
        {
            memcpy(address, interface->dns, 4);
            return 0;
        }
    }
    errno = ENETUNREACH;
    return -1;
}

static int stream_call(unsigned operation, struct lume_stream_request *request)
{
    return native_result(
        native_call(LUME_SYS_STREAM, operation, (uintptr_t)request, sizeof(*request)));
}

static int wait_ready(struct lume_stream_request *request, unsigned event, uint64_t deadline)
{
    while ((uint64_t)native_call(LUME_SYS_CLOCK, 0, 0, 0) < deadline)
    {
        int flags = stream_call(LUME_STREAM_POLL, request);
        if (flags < 0)
            return -1;
        if ((unsigned)flags & event)
            return 0;
        if (flags & (LUME_STREAM_ERROR | LUME_STREAM_HANGUP))
        {
            errno = ECONNRESET;
            return -1;
        }
        if (native_result(native_call(LUME_SYS_SLEEP, 10, 0, 0)) < 0)
            return -1;
    }
    errno = ETIMEDOUT;
    return -1;
}

static int transfer(struct lume_stream_request *request, unsigned operation, unsigned char *data,
                    unsigned length, uint64_t deadline)
{
    unsigned position = 0;
    unsigned event = operation == LUME_STREAM_SEND ? LUME_STREAM_WRITABLE : LUME_STREAM_READABLE;
    while (position < length)
    {
        if (wait_ready(request, event, deadline))
            return -1;
        unsigned count = length - position;
        if (count > sizeof(request->data))
            count = sizeof(request->data);
        if (operation == LUME_STREAM_SEND)
            memcpy(request->data, data + position, count);
        request->count = count;
        int result = stream_call(operation, request);
        if (result < 0 && errno == EAGAIN)
            continue;
        if (result <= 0 || (unsigned)result > count)
        {
            if (!result)
                errno = ECONNRESET;
            return -1;
        }
        if (operation == LUME_STREAM_RECEIVE)
            memcpy(data + position, request->data, (unsigned)result);
        position += (unsigned)result;
    }
    return 0;
}

static int tcp_query(const unsigned char server[4], unsigned char *query, unsigned length,
                     unsigned char *answer, unsigned capacity, uint64_t deadline)
{
    struct lume_stream_request request = {.version = LUME_IO_VERSION, .port = 53};
    memcpy(request.address, server, 4);
    if (stream_call(LUME_STREAM_CONNECT_ASYNC, &request) < 0)
        return -1;
    unsigned char size[2];
    ns_put16(length, size);
    int result = -1;
    if (transfer(&request, LUME_STREAM_SEND, size, 2, deadline) ||
        transfer(&request, LUME_STREAM_SEND, query, length, deadline) ||
        transfer(&request, LUME_STREAM_RECEIVE, size, 2, deadline))
        goto finished;
    unsigned bytes = ns_get16(size);
    if (bytes < 12)
    {
        errno = EBADMSG;
        goto finished;
    }
    if (bytes > capacity)
        result = (int)bytes;
    else if (!transfer(&request, LUME_STREAM_RECEIVE, answer, bytes, deadline))
        result = (int)bytes;
finished:
    {
        int saved = errno;
        (void)stream_call(LUME_STREAM_CLOSE, &request);
        errno = saved;
    }
    return result;
}

static int matching_question(const unsigned char *query, unsigned query_bytes,
                             const unsigned char *answer, unsigned answer_bytes)
{
    if (answer_bytes < 16 || memcmp(query, answer, 2) || (answer[2] & 0xf8) != 0x80 ||
        ns_get16(answer + 4) != 1)
        return 0;
    char question[256], response[256];
    int sent = dn_expand(query, query + query_bytes, query + 12, question, sizeof(question));
    int received =
        dn_expand(answer, answer + answer_bytes, answer + 12, response, sizeof(response));
    return sent > 0 && received > 0 && (unsigned)received <= answer_bytes - 16 &&
           !strcasecmp(question, response) && !memcmp(query + 12 + sent, answer + 12 + received, 4);
}

int res_query(const char *name, int qclass, int type, unsigned char *answer, int capacity)
{
    unsigned char query[272], server[4];
    int length = query_name(name, query, qclass, type);
    h_errno = NO_RECOVERY;
    if (length < 0 || !answer || capacity < 12)
    {
        errno = EINVAL;
        return -1;
    }
    h_errno = TRY_AGAIN;
    if (dns_server(server))
        return -1;
    struct lume_stream_request request = {.version = LUME_IO_VERSION, .port = 53};
    memcpy(request.address, server, 4);
    if (stream_call(LUME_DATAGRAM_CONNECT, &request) < 0)
        return -1;
    uint64_t deadline = (uint64_t)native_call(LUME_SYS_CLOCK, 0, 0, 0) + 5000;
    int bytes = -1;
    memcpy(request.data, query, (unsigned)length);
    request.count = (unsigned)length;
    if (stream_call(LUME_STREAM_SEND, &request) != length)
        goto closed;
    while (!wait_ready(&request, LUME_STREAM_READABLE, deadline))
    {
        request.count = sizeof(request.data);
        bytes = stream_call(LUME_STREAM_RECEIVE, &request);
        if (bytes < 0 && errno == EAGAIN)
            continue;
        if (bytes < 0)
            break;
        if (!matching_question(query, (unsigned)length, request.data, (unsigned)bytes))
        {
            bytes = -1;
            continue;
        }
        if (request.data[2] & 2)
        {
            (void)stream_call(LUME_STREAM_CLOSE, &request);
            bytes =
                tcp_query(server, query, (unsigned)length, answer, (unsigned)capacity, deadline);
            if (bytes > capacity)
                return bytes;
            if (bytes < 0)
                return -1;
            if (!matching_question(query, (unsigned)length, answer, (unsigned)bytes) ||
                (answer[2] & 2))
            {
                errno = EBADMSG;
                h_errno = NO_RECOVERY;
                return -1;
            }
            goto received;
        }
        memcpy(answer, request.data,
               (unsigned)bytes < (unsigned)capacity ? (unsigned)bytes : (unsigned)capacity);
        break;
    }
closed:
    {
        int saved = errno;
        (void)stream_call(LUME_STREAM_CLOSE, &request);
        errno = saved;
    }
    if (bytes < 0)
        return -1;
received:
    {
        unsigned status = answer[3] & 15;
        if (status || !ns_get16(answer + 6))
        {
            h_errno = status == 3   ? HOST_NOT_FOUND
                      : status == 2 ? TRY_AGAIN
                      : status      ? NO_RECOVERY
                                    : NO_DATA;
            return -1;
        }
    }
    h_errno = 0;
    return bytes;
}
