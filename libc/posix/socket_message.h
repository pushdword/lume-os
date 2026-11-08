/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_SOCKET_MESSAGE_H
#define LUME_SOCKET_MESSAGE_H

#define MESSAGE_SCRATCH_BYTES 4096u

static _Thread_local unsigned char message_scratch[MESSAGE_SCRATCH_BYTES];
static _Thread_local bool message_scratch_busy;

struct message_buffer
{
    unsigned char *data;
    bool scratch;
};

static struct message_buffer message_buffer_acquire(size_t bytes)
{
    if (!bytes)
        return (struct message_buffer){0};
    if (bytes <= sizeof(message_scratch) && !message_scratch_busy)
    {
        message_scratch_busy = true;
        return (struct message_buffer){message_scratch, true};
    }
    return (struct message_buffer){malloc(bytes), false};
}

static void message_buffer_release(struct message_buffer *buffer)
{
    int saved = errno;
    if (buffer->scratch)
        message_scratch_busy = false;
    else if (buffer->data)
        free(buffer->data);
    *buffer = (struct message_buffer){0};
    errno = saved;
}

static int message_rights(const struct msghdr *message, struct lume_socket_message *request)
{
    if (message->msg_controllen && !message->msg_control)
    {
        errno = EFAULT;
        return -1;
    }
    const unsigned char *control = message->msg_control;
    size_t offset = 0;
    while (message->msg_controllen - offset >= sizeof(struct cmsghdr))
    {
        struct cmsghdr header;
        memcpy(&header, control + offset, sizeof(header));
        size_t remaining = message->msg_controllen - offset;
        if (header.cmsg_len < CMSG_LEN(0) || header.cmsg_len > remaining)
        {
            errno = EINVAL;
            return -1;
        }
        if (header.cmsg_level != SOL_SOCKET || header.cmsg_type != SCM_RIGHTS)
        {
            errno = EOPNOTSUPP;
            return -1;
        }
        size_t bytes = header.cmsg_len - CMSG_LEN(0);
        if (bytes % sizeof(int))
        {
            errno = EINVAL;
            return -1;
        }
        size_t count = bytes / sizeof(int);
        if (count > LUME_SOCKET_RIGHTS_MAX - request->rights_count)
        {
            errno = EMSGSIZE;
            return -1;
        }
        memcpy(request->rights + request->rights_count, control + offset + CMSG_LEN(0), bytes);
        request->rights_count += (unsigned)count;
        if (header.cmsg_len > SIZE_MAX - sizeof(size_t) + 1)
            break;
        size_t next = CMSG_ALIGN(header.cmsg_len);
        if (next > remaining)
            break;
        offset += next;
    }
    return 0;
}

static ssize_t message_io(int fd, const struct msghdr *message, struct msghdr *received, int flags)
{
    bool sending = received == NULL;
    int allowed =
        MSG_DONTWAIT | MSG_NOSIGNAL | (sending ? 0 : MSG_PEEK | MSG_CMSG_CLOEXEC | MSG_TRUNC);
    if (!message)
    {
        errno = EFAULT;
        return -1;
    }
    struct msghdr input = *message;
    message = &input;
    if (message->msg_iovlen && !message->msg_iov)
    {
        errno = EFAULT;
        return -1;
    }
    if ((flags & ~allowed) || message->msg_iovlen > 1024)
    {
        errno = EOPNOTSUPP;
        return -1;
    }
    struct iovec vectors[message->msg_iovlen ? message->msg_iovlen : 1];
    if (message->msg_iovlen)
        memcpy(vectors, message->msg_iov, message->msg_iovlen * sizeof(*vectors));
    input.msg_iov = vectors;
    struct lume_socket_message request = {.version = LUME_SOCKET_MESSAGE_VERSION,
                                          .bytes = sizeof(request)};
    unsigned rights_capacity = 0;
    size_t control_capacity = message->msg_controllen;
    if (sending)
    {
        if (message_rights(message, &request))
            return -1;
    }
    else
    {
        if (control_capacity && !message->msg_control)
        {
            errno = EFAULT;
            return -1;
        }
        if (control_capacity >= CMSG_LEN(0))
        {
            size_t capacity = (control_capacity - CMSG_LEN(0)) / sizeof(int);
            rights_capacity =
                capacity < LUME_SOCKET_RIGHTS_MAX ? (unsigned)capacity : LUME_SOCKET_RIGHTS_MAX;
        }
        request.flags = (flags & MSG_PEEK ? LUME_SOCKET_MESSAGE_PEEK : 0) |
                        (flags & MSG_CMSG_CLOEXEC ? LUME_SOCKET_MESSAGE_CLOEXEC : 0);
        received->msg_flags = 0;
        received->msg_controllen = 0;
    }
    size_t bytes = 0;
    unsigned nonempty = 0;
    void *single = NULL;
    for (size_t i = 0; i < message->msg_iovlen; ++i)
    {
        if (message->msg_iov[i].iov_len > SIZE_MAX / 2 - bytes)
        {
            errno = EINVAL;
            return -1;
        }
        if (message->msg_iov[i].iov_len)
        {
            single = message->msg_iov[i].iov_base;
            ++nonempty;
        }
        bytes += message->msg_iov[i].iov_len;
    }
    if (!bytes && request.rights_count)
    {
        errno = EINVAL;
        return -1;
    }
    if (bytes > LUME_FILE_IO_MAX)
        bytes = LUME_FILE_IO_MAX;
    struct message_buffer lease = {0};
    if (nonempty != 1)
        lease = message_buffer_acquire(bytes);
    unsigned char *buffer = nonempty == 1 ? single : lease.data;
    if (bytes && !buffer)
    {
        if (nonempty == 1)
            errno = EFAULT;
        return -1;
    }
    if (sending && nonempty != 1)
    {
        size_t offset = 0;
        for (size_t i = 0; i < message->msg_iovlen && offset < bytes; ++i)
        {
            size_t length = message->msg_iov[i].iov_len;
            if (length > bytes - offset)
                length = bytes - offset;
            memcpy(buffer + offset, message->msg_iov[i].iov_base, length);
            offset += length;
        }
    }
    request.buffer = (uintptr_t)buffer;
    request.count = (unsigned)bytes;
    struct lume_socket_request call = {
        .fd = fd, .buffer = (uintptr_t)&request, .count = sizeof(request)};
    unsigned operation = sending ? LUME_SOCKET_SEND_MESSAGE : LUME_SOCKET_RECEIVE_MESSAGE;
    bool addressed = message->msg_name != NULL;
    if (addressed)
    {
        if (sending)
        {
            char path[108];
            if (request.rights_count ||
                address_request(fd, message->msg_name, message->msg_namelen, &call, path))
            {
                if (request.rights_count)
                    errno = EOPNOTSUPP;
                if (nonempty != 1)
                    message_buffer_release(&lease);
                return -1;
            }
            if (call.family != AF_INET || !call.port)
            {
                if (nonempty != 1)
                    message_buffer_release(&lease);
                errno = call.family != AF_INET ? EAFNOSUPPORT : EINVAL;
                return -1;
            }
            operation = LUME_SOCKET_SENDTO;
        }
        else
        {
            call = (struct lume_socket_request){
                .fd = fd,
                .family = AF_INET,
                .flags = flags & MSG_PEEK ? LUME_SOCKET_DATAGRAM_PEEK : 0};
            operation = LUME_SOCKET_RECVFROM;
        }
        call.buffer = (uintptr_t)buffer;
        call.count = bytes;
    }
    int result;
    for (;;)
    {
        if (!sending)
            request.rights_count = rights_capacity;
        result = socket_call(operation, &call);
        if (result >= 0 || errno != EAGAIN || (flags & MSG_DONTWAIT) ||
            (fcntl(fd, F_GETFL) & O_NONBLOCK))
            break;
        if (ready(fd, sending ? POLLOUT : POLLIN))
        {
            result = -1;
            break;
        }
    }
    int saved = errno;
    int reported = result;
    if (!sending && result >= 0)
    {
        if (addressed)
        {
            request.rights_count = 0;
            request.count = call.count;
            request.received_flags =
                call.flags & LUME_SOCKET_DATAGRAM_TRUNCATED ? LUME_SOCKET_MESSAGE_TRUNCATED : 0;
        }
        if (received->msg_name && call.family == AF_INET)
            (void)return_address(&call, received->msg_name, &received->msg_namelen);
        else
            received->msg_namelen = 0;
        if ((flags & MSG_TRUNC) && call.family == AF_INET &&
            (request.received_flags & LUME_SOCKET_MESSAGE_TRUNCATED))
            reported = request.count;

        if (nonempty != 1)
        {
            size_t offset = 0;
            for (size_t i = 0; i < message->msg_iovlen && offset < (unsigned)result; ++i)
            {
                size_t length = message->msg_iov[i].iov_len;
                if (length > (unsigned)result - offset)
                    length = (unsigned)result - offset;
                memcpy(message->msg_iov[i].iov_base, buffer + offset, length);
                offset += length;
            }
        }
        if (request.rights_count)
        {
            size_t payload = request.rights_count * sizeof(int);
            size_t used = CMSG_SPACE(payload);
            if (used > control_capacity)
                used = CMSG_LEN(payload);
            memset(received->msg_control, 0, used);
            struct cmsghdr *header = received->msg_control;
            header->cmsg_len = CMSG_LEN(payload);
            header->cmsg_level = SOL_SOCKET;
            header->cmsg_type = SCM_RIGHTS;
            memcpy(CMSG_DATA(header), request.rights, payload);
            received->msg_controllen = used;
        }
        received->msg_flags =
            (request.received_flags & LUME_SOCKET_MESSAGE_CTRUNC ? MSG_CTRUNC : 0) |
            (request.received_flags & LUME_SOCKET_MESSAGE_TRUNCATED ? MSG_TRUNC : 0);
        if (call.family == AF_INET && (call.flags & LUME_SOCKET_DATAGRAM_TOS))
        {
            if (control_capacity < CMSG_LEN(sizeof(uint8_t)))
                received->msg_flags |= MSG_CTRUNC;
            else
            {
                size_t used = CMSG_SPACE(sizeof(uint8_t));
                if (used > control_capacity)
                    used = CMSG_LEN(sizeof(uint8_t));
                memset(received->msg_control, 0, used);
                struct cmsghdr *header = received->msg_control;
                header->cmsg_len = CMSG_LEN(sizeof(uint8_t));
                header->cmsg_level = IPPROTO_IP;
                header->cmsg_type = IP_TOS;
                *(uint8_t *)CMSG_DATA(header) = (uint8_t)call.type;
                received->msg_controllen = used;
            }
        }
    }
    if (nonempty != 1)
        message_buffer_release(&lease);
    errno = saved;
    return reported;
}

#endif
