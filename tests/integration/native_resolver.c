/* SPDX-License-Identifier: BSD-3-Clause */
#include <stdint.h>
#define LUME_ARCH_SYSCALL_NATIVE_H
static int64_t lume_native_call(uint64_t, uint64_t, uint64_t, uint64_t);
#include "../../libc/posix/resolver.c"
#include <assert.h>
#include <stdio.h>

enum
{
    VALID,
    WRONG_ID,
    NAME_NOT_FOUND,
    NO_ANSWER,
    SERVER_FAILURE,
    TCP_FALLBACK,
    TCP_WRONG_ID,
    TCP_EOF,
    TIMEOUT,
    SHORT_REPLY
};

static unsigned scenario, opened, closed, receives, tcp_sent, tcp_read;
static unsigned char sent[300], reply[512], framed[514];
static unsigned sent_bytes, reply_bytes;
static uint64_t now;

static void response(void)
{
    memcpy(reply, sent, sent_bytes);
    reply[2] = 0x81;
    reply[3] = 0x80;
    reply[7] = 1;
    unsigned char record[] = {0xc0, 0x0c, 0, 1, 0, 1, 0, 0, 0, 30, 0, 4, 192, 0, 2, 9};
    memcpy(reply + sent_bytes, record, sizeof(record));
    reply_bytes = sent_bytes + sizeof(record);
}

static int64_t lume_native_call(uint64_t number, uint64_t operation, uint64_t argument,
                                uint64_t count)
{
    if (number == LUME_SYS_CLOCK)
        return (int64_t)now++;
    if (number == LUME_SYS_SLEEP)
    {
        now += operation;
        return 0;
    }
    if (number == LUME_SYS_RANDOM)
    {
        assert(argument == 2);
        memset((void *)(uintptr_t)operation, 0x71, 2);
        return 2;
    }
    if (number == LUME_SYS_NETCTL)
    {
        assert(operation == LUME_NET_ENUM && count == sizeof(struct lume_net_control));
        struct lume_net_control *request = (void *)(uintptr_t)argument;
        request->count = 1;
        request->data.interfaces[0].flags = LUME_IF_UP | LUME_IF_LINK;
        memcpy(request->data.interfaces[0].dns, "\x0a\0\x02\x03", 4);
        return 0;
    }
    assert(number == LUME_SYS_STREAM && count == sizeof(struct lume_stream_request));
    struct lume_stream_request *request = (void *)(uintptr_t)argument;
    if (operation == LUME_DATAGRAM_CONNECT || operation == LUME_STREAM_CONNECT_ASYNC)
    {
        request->handle = operation == LUME_DATAGRAM_CONNECT ? 1 : 2;
        ++opened;
        return 0;
    }
    if (operation == LUME_STREAM_CLOSE)
    {
        ++closed;
        return 0;
    }
    if (operation == LUME_STREAM_POLL)
        return scenario == TIMEOUT ? 0 : LUME_STREAM_READABLE | LUME_STREAM_WRITABLE;
    if (operation == LUME_STREAM_SEND)
    {
        if (request->handle == 1)
        {
            sent_bytes = request->count;
            memcpy(sent, request->data, sent_bytes);
            response();
            return request->count;
        }
        unsigned bytes = request->count > 3 ? 3 : request->count;
        assert(tcp_sent + bytes <= sizeof(framed));
        memcpy(framed + tcp_sent, request->data, bytes);
        tcp_sent += bytes;
        return bytes;
    }
    assert(operation == LUME_STREAM_RECEIVE);
    ++receives;
    if (request->handle == 1)
    {
        memcpy(request->data, reply, reply_bytes);
        if (scenario == SHORT_REPLY)
        {
            request->count = 15;
            return 15;
        }
        if (scenario == WRONG_ID && receives == 1)
            request->data[0] ^= 1;
        if (scenario == NAME_NOT_FOUND)
            request->data[3] |= 3;
        if (scenario == NO_ANSWER)
            request->data[7] = 0;
        if (scenario == SERVER_FAILURE)
            request->data[3] |= 2;
        if (scenario >= TCP_FALLBACK && scenario <= TCP_EOF)
            request->data[2] |= 2;
        request->count = reply_bytes;
        return reply_bytes;
    }
    assert(tcp_sent == sent_bytes + 2 && ns_get16(framed) == sent_bytes);
    assert(!memcmp(framed + 2, sent, sent_bytes));
    if (scenario == TCP_EOF)
        return 0;
    unsigned char wire[514];
    ns_put16(reply_bytes, wire);
    memcpy(wire + 2, reply, reply_bytes);
    if (scenario == TCP_WRONG_ID)
        wire[2] ^= 1;
    unsigned bytes = request->count > 7 ? 7 : request->count;
    if (bytes > reply_bytes + 2 - tcp_read)
        bytes = reply_bytes + 2 - tcp_read;
    memcpy(request->data, wire + tcp_read, bytes);
    tcp_read += bytes;
    request->count = bytes;
    return bytes;
}

static int lookup(unsigned test, int capacity, unsigned char *answer)
{
    scenario = test;
    opened = closed = receives = tcp_sent = tcp_read = 0;
    now = 0;
    int result = res_query("cafe.example.", C_IN, T_A, answer, capacity);
    assert(opened == closed);
    return result;
}

int main(void)
{
    unsigned char answer[512];
    int bytes = lookup(VALID, sizeof(answer), answer);
    assert(bytes == (int)reply_bytes && !h_errno);
    ns_msg parsed;
    ns_rr record;
    assert(!ns_initparse(answer, bytes, &parsed));
    assert(!ns_parserr(&parsed, ns_s_an, 0, &record));
    assert(!strcmp(ns_rr_name(record), "cafe.example"));
    assert(ns_rr_type(record) == T_A && ns_rr_rdlen(record) == 4);
    assert(!memcmp(ns_rr_rdata(record), "\xc0\0\x02\x09", 4));
    assert(lookup(WRONG_ID, sizeof(answer), answer) == bytes && receives == 2);
    assert(lookup(NAME_NOT_FOUND, sizeof(answer), answer) < 0 && h_errno == HOST_NOT_FOUND);
    assert(lookup(NO_ANSWER, sizeof(answer), answer) < 0 && h_errno == NO_DATA);
    assert(lookup(SERVER_FAILURE, sizeof(answer), answer) < 0 && h_errno == TRY_AGAIN);
    assert(lookup(TCP_FALLBACK, sizeof(answer), answer) == bytes && opened == 2);
    assert(lookup(TCP_WRONG_ID, sizeof(answer), answer) < 0 && h_errno == NO_RECOVERY);
    assert(lookup(TCP_EOF, sizeof(answer), answer) < 0 && errno == ECONNRESET);
    assert(lookup(TIMEOUT, sizeof(answer), answer) < 0 && errno == ETIMEDOUT);
    assert(lookup(SHORT_REPLY, sizeof(answer), answer) < 0 && errno == ETIMEDOUT);
    assert(lookup(VALID, 16, answer) == bytes);
    assert(lookup(TCP_FALLBACK, 16, answer) == bytes && opened == 2);
    assert(res_query("a..", C_IN, T_A, answer, sizeof(answer)) < 0);
    char long_label[66];
    memset(long_label, 'a', 64);
    long_label[64] = 0;
    assert(res_query(long_label, C_IN, T_A, answer, sizeof(answer)) < 0);
    assert(res_query(NULL, C_IN, T_A, answer, sizeof(answer)) < 0);
    assert(res_query("example", C_IN, T_A, answer, 11) < 0);
    unsigned char loop[] = {0xc0, 0};
    char expanded[256];
    assert(dn_expand(loop, loop + sizeof(loop), loop, expanded, sizeof(expanded)) < 0);
    assert(dn_expand(loop, loop + 1, loop, expanded, sizeof(expanded)) < 0);
    puts("RESOLVER_NATIVE_PASS");
    return 0;
}
