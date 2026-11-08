#define WIN32_LEAN_AND_MEAN
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifdef _WIN32
#include <process.h>
#include <windows.h>
#endif
#define uid_t uint32_t
#define gid_t uint32_t
#include "../../include/posix/sys/socket.h"
#undef uid_t
#undef gid_t
#include "../../include/posix/sys/uio.h"
#include "../../include/posix/netinet/in.h"
#include "lume/abi.h"
#include "lume/socket.h"
#include "fd.h"

/* The host fixture supplies POSIX control constants absent from the Windows CRT. */
#define F_GETFL 3
#define O_NONBLOCK 0x4000
#define POLLIN LUME_POLLIN
#define POLLOUT LUME_POLLOUT

static unsigned allocations, releases, calls, successful_sends, copied;
static size_t limit = SIZE_MAX;
static int wire_error;
static bool allocation_error, nested;
static bool peek, truncate_packet;
static unsigned incoming_rights;
static unsigned char captured[LUME_FILE_IO_MAX];
static size_t captured_bytes;
static struct iovec *changed_vectors;
static char replacement[64];
static _Thread_local unsigned char thread_tag;
#ifdef _WIN32
static HANDLE rendezvous;
static unsigned arrived;
#endif

static ssize_t message_io(int, const struct msghdr *, struct msghdr *, int);

static void *test_malloc(size_t bytes)
{
    __atomic_add_fetch(&allocations, 1, __ATOMIC_RELAXED);
    if (allocation_error)
    {
        errno = ENOMEM;
        return NULL;
    }
    return malloc(bytes);
}

static void test_free(void *pointer)
{
    __atomic_add_fetch(&releases, 1, __ATOMIC_RELAXED);
    free(pointer);
    errno = ERANGE;
}

static void *test_memcpy(void *out, const void *in, size_t bytes)
{
    __atomic_add_fetch(&copied, (unsigned)bytes, __ATOMIC_RELAXED);
    return memcpy(out, in, bytes);
}

static int test_fcntl(int fd, int operation)
{
    assert(fd == 7 && operation == F_GETFL);
    return O_NONBLOCK;
}

static int ready(int fd, short events)
{
    (void)fd;
    (void)events;
    assert(false);
    return -1;
}

static int address_request(int fd, const struct sockaddr *address, socklen_t bytes,
                           struct lume_socket_request *request, char path[108])
{
    (void)path;
    assert(address && bytes == sizeof(*address));
    *request = (struct lume_socket_request){.fd = fd, .family = address->sa_family, .port = 8080};
    return 0;
}

static int return_address(const struct lume_socket_request *request, struct sockaddr *address,
                          socklen_t *bytes)
{
    assert(request->family == AF_INET && address && bytes && *bytes >= sizeof(*address));
    *address = (struct sockaddr){.sa_family = AF_INET};
    *bytes = sizeof(*address);
    return 0;
}

static int socket_call(unsigned operation, struct lume_socket_request *call)
{
    assert(call->fd == 7);
    __atomic_add_fetch(&calls, 1, __ATOMIC_RELAXED);
    if (wire_error)
    {
        errno = wire_error;
        return -1;
    }
    struct lume_socket_message *message = (void *)(uintptr_t)call->buffer;
    bool datagram = operation == LUME_SOCKET_SENDTO || operation == LUME_SOCKET_RECVFROM;
    bool sending = operation == LUME_SOCKET_SEND_MESSAGE || operation == LUME_SOCKET_SENDTO;
    assert(sending || operation == LUME_SOCKET_RECEIVE_MESSAGE ||
           operation == LUME_SOCKET_RECVFROM);
    size_t bytes = datagram ? call->count : message->count;
    unsigned char *buffer = (void *)(uintptr_t)(datagram ? call->buffer : message->buffer);
    assert(bytes <= sizeof(captured));
    size_t result = bytes < limit ? bytes : limit;
    if (sending)
    {
        if (nested)
        {
            nested = false;
            char inner[] = "inner";
            struct iovec vectors[] = {{inner, 2}, {inner + 2, 3}};
            struct msghdr inner_message = {.msg_iov = vectors, .msg_iovlen = 2};
            assert(message_io(7, &inner_message, NULL, 0) == 5);
        }
        if (thread_tag)
        {
#ifdef _WIN32
            if (__atomic_add_fetch(&arrived, 1, __ATOMIC_ACQ_REL) == 2)
                assert(SetEvent(rendezvous));
            assert(WaitForSingleObject(rendezvous, 5000) == WAIT_OBJECT_0);
#endif
            for (size_t i = 0; i < result; ++i)
                assert(buffer[i] == thread_tag);
        }
        else
        {
            memcpy(captured, buffer, result);
            captured_bytes = result;
        }
        __atomic_add_fetch(&successful_sends, 1, __ATOMIC_RELAXED);
    }
    else
    {
        memset(buffer, 'r', result);
        if (changed_vectors)
        {
            changed_vectors[0] = (struct iovec){replacement, sizeof(replacement)};
            changed_vectors[1] = (struct iovec){replacement, sizeof(replacement)};
        }
        if (datagram)
        {
            assert(!!(call->flags & LUME_SOCKET_DATAGRAM_PEEK) == peek);
            call->family = AF_INET;
            call->count = truncate_packet ? (unsigned)result + 9 : (unsigned)result;
            call->flags = truncate_packet ? LUME_SOCKET_DATAGRAM_TRUNCATED : 0;
        }
        else
        {
            assert(!!(message->flags & LUME_SOCKET_MESSAGE_PEEK) == peek);
            unsigned capacity = message->rights_count;
            message->rights_count = incoming_rights < capacity ? incoming_rights : capacity;
            for (unsigned i = 0; i < message->rights_count; ++i)
                message->rights[i] = 40 + (int)i;
            message->received_flags = incoming_rights > capacity ? LUME_SOCKET_MESSAGE_CTRUNC : 0;
        }
    }
    errno = ECHILD;
    return (int)result;
}

#define malloc test_malloc
#define free test_free
#define memcpy test_memcpy
#define fcntl test_fcntl
#include "../../libc/posix/socket_message.h"
#undef malloc
#undef free
#undef memcpy
#undef fcntl

static void reset(void)
{
    allocations = releases = calls = successful_sends = copied = 0;
    limit = SIZE_MAX;
    wire_error = 0;
    allocation_error = nested = peek = truncate_packet = false;
    incoming_rights = 0;
    changed_vectors = NULL;
    captured_bytes = 0;
    memset(captured, 0, sizeof(captured));
    errno = EBUSY;
}

static void compatibility(void)
{
    char data[8192];
    memset(data, 'x', sizeof(data));
    struct iovec vectors[] = {{data, 8}, {data + 8, 16}};
    struct msghdr message = {.msg_iov = vectors, .msg_iovlen = 2};
    reset();
    assert(message_io(7, &message, NULL, MSG_NOSIGNAL) == 24 && calls == 1);
    assert(captured_bytes == 24 && !memcmp(captured, data, 24) && errno == ECHILD);
    reset();
    limit = 11;
    assert(message_io(7, &message, &message, 0) == 11 && calls == 1);
    assert(data[10] == 'r' && data[11] == 'x' && errno == ECHILD);
    reset();
    wire_error = EAGAIN;
    assert(message_io(7, &message, &message, MSG_DONTWAIT) == -1 && errno == EAGAIN && calls == 1);
    reset();
    struct iovec one = {data, 24};
    message = (struct msghdr){.msg_iov = &one, .msg_iovlen = 1};
    assert(message_io(7, &message, NULL, 0) == 24 && !allocations);
    reset();
    one.iov_len = SIZE_MAX;
    assert(message_io(7, &message, NULL, 0) == -1 && errno == EINVAL && !calls);
    reset();
    message.msg_iovlen = SIZE_MAX;
    assert(message_io(7, &message, NULL, 0) == -1 && errno == EOPNOTSUPP && !calls);
    reset();
    vectors[0].iov_len = vectors[1].iov_len = 4096;
    message = (struct msghdr){.msg_iov = vectors, .msg_iovlen = 2};
    allocation_error = true;
    assert(message_io(7, &message, NULL, 0) == -1 && errno == ENOMEM && !calls);
    reset();
    limit = 5000;
    assert(message_io(7, &message, NULL, 0) == 5000 && calls == 1);
    assert(allocations == 1 && releases == 1 && errno == ECHILD);
    reset();
    vectors[0].iov_len = 8;
    vectors[1].iov_len = 16;
    _Alignas(size_t) unsigned char control[CMSG_SPACE(2 * sizeof(int))] = {0};
    struct cmsghdr *rights = (void *)control;
    *rights = (struct cmsghdr){CMSG_LEN(2 * sizeof(int)), SOL_SOCKET, SCM_RIGHTS};
    int descriptors[] = {20, 21};
    memcpy(CMSG_DATA(rights), descriptors, sizeof(descriptors));
    message = (struct msghdr){.msg_iov = vectors,
                              .msg_iovlen = 2,
                              .msg_control = control,
                              .msg_controllen = sizeof(control)};
    assert(message_io(7, &message, NULL, 0) == 24 && calls == 1 && successful_sends == 1);
    reset();
    incoming_rights = 3;
    peek = true;
    message.msg_controllen = sizeof(control);
    assert(message_io(7, &message, &message, MSG_PEEK | MSG_CMSG_CLOEXEC) == 24);
    assert(message.msg_flags == MSG_CTRUNC && message.msg_controllen == sizeof(control));
    int *received = (void *)CMSG_DATA((struct cmsghdr *)(void *)control);
    assert(received[0] == 40 && received[1] == 41 && calls == 1);
    reset();
    struct sockaddr source = {.sa_family = AF_INET};
    message = (struct msghdr){
        .msg_iov = vectors, .msg_iovlen = 2, .msg_name = &source, .msg_namelen = sizeof(source)};
    assert(message_io(7, &message, NULL, 0) == 24 && calls == 1);
    reset();
    limit = 9;
    truncate_packet = true;
    assert(message_io(7, &message, &message, MSG_TRUNC) == 18 && calls == 1);
    assert(message.msg_flags == MSG_TRUNC && message.msg_namelen == sizeof(source));
}

static void descriptor_snapshot(void)
{
    char first[8], second[16];
    memset(first, 'a', sizeof(first));
    memset(second, 'b', sizeof(second));
    struct iovec vectors[] = {{first, sizeof(first)}, {second, sizeof(second)}};
    struct msghdr message = {.msg_iov = vectors, .msg_iovlen = 2};
    reset();
    changed_vectors = vectors;
    limit = 11;
    assert(message_io(7, &message, &message, 0) == 11);
    for (unsigned i = 0; i < sizeof(first); ++i)
        assert(first[i] == 'r');
    assert(second[0] == 'r' && second[2] == 'r' && second[3] == 'b');
}

#ifdef _WIN32
static unsigned __stdcall thread_send(void *argument)
{
    thread_tag = (unsigned char)(uintptr_t)argument;
    unsigned char payload[64];
    memset(payload, thread_tag, sizeof(payload));
    struct iovec vectors[] = {{payload, 4}, {payload + 4, 60}};
    struct msghdr message = {.msg_iov = vectors, .msg_iovlen = 2};
    assert(message_io(7, &message, NULL, 0) == sizeof(payload));
    return 0;
}

static void thread_isolation(void)
{
    reset();
    arrived = 0;
    rendezvous = CreateEvent(NULL, TRUE, FALSE, NULL);
    assert(rendezvous);
    HANDLE first = (HANDLE)_beginthreadex(NULL, 0, thread_send, (void *)(uintptr_t)'a', 0, NULL);
    HANDLE second = (HANDLE)_beginthreadex(NULL, 0, thread_send, (void *)(uintptr_t)'b', 0, NULL);
    assert(first && second);
    HANDLE threads[] = {first, second};
    assert(WaitForMultipleObjects(2, threads, TRUE, 5000) == WAIT_OBJECT_0);
    assert(CloseHandle(first) && CloseHandle(second) && CloseHandle(rendezvous));
    assert(calls == 2 && successful_sends == 2);
}
#endif

int main(int argc, char **argv)
{
    bool baseline = argc == 2 && !strcmp(argv[1], "--baseline");
    compatibility();
    if (!baseline)
        descriptor_snapshot();
    reset();
    char payload[64];
    memset(payload, 'm', sizeof(payload));
    struct iovec vectors[] = {{payload, 8}, {payload + 8, 56}};
    struct msghdr message = {.msg_iov = vectors, .msg_iovlen = 2};
    for (unsigned i = 0; i < 10000; ++i)
        assert(message_io(7, &message, NULL, 0) == 64);
    printf("MESSAGE_COST calls=%u allocations=%u releases=%u copied=%u\n", calls, allocations,
           releases, copied);
    assert(calls == 10000 && successful_sends == calls);
    assert(baseline ? allocations == calls && releases == calls : !allocations && !releases);
    reset();
    nested = true;
    assert(message_io(7, &message, NULL, 0) == 64 && calls == 2);
    assert(captured_bytes == sizeof(payload) && !memcmp(captured, payload, sizeof(payload)));
    assert(allocations == (baseline ? 2u : 1u) && releases == allocations);
#ifdef _WIN32
    thread_isolation();
#endif
    puts("PASS_SOCKET_MESSAGE bounds, partial I/O, rights, packets and independent buffers");
    return 0;
}
