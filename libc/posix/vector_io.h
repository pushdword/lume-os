/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_VECTOR_IO_H
#define LUME_VECTOR_IO_H

static ssize_t vector_io(int fd, const struct iovec *vectors, int count, int writing)
{
    if (count < 0 || count > 1024)
    {
        errno = EINVAL;
        return -1;
    }
    size_t bytes = 0;
    const struct iovec *single = NULL;
    unsigned nonempty = 0;
    for (int i = 0; i < count; ++i)
    {
        if (vectors[i].iov_len > SIZE_MAX / 2 - bytes)
        {
            errno = EINVAL;
            return -1;
        }
        if (vectors[i].iov_len)
        {
            single = &vectors[i];
            ++nonempty;
        }
        bytes += vectors[i].iov_len;
    }
    if (!bytes)
        return 0;
    if (bytes > 65536)
        bytes = 65536;
    if (nonempty == 1)
        return writing ? write(fd, single->iov_base, bytes) : read(fd, single->iov_base, bytes);
#ifdef LUME_VECTOR_NATIVE
    int handled;
    ssize_t native = native_vector_io(fd, vectors, count, writing, &handled);
    if (handled)
        return native;
#endif
    char *buffer = malloc(bytes);
    if (!buffer)
        return -1;
    size_t offset = 0;
    if (writing)
        for (int i = 0; i < count && offset < bytes; ++i)
        {
            size_t length = vectors[i].iov_len;
            if (length > bytes - offset)
                length = bytes - offset;
            memcpy(buffer + offset, vectors[i].iov_base, length);
            offset += length;
        }
    ssize_t result = writing ? write(fd, buffer, bytes) : read(fd, buffer, bytes);
    if (!writing && result > 0)
    {
        offset = 0;
        for (int i = 0; i < count && offset < (size_t)result; ++i)
        {
            size_t length = vectors[i].iov_len;
            if (length > (size_t)result - offset)
                length = (size_t)result - offset;
            memcpy(vectors[i].iov_base, buffer + offset, length);
            offset += length;
        }
    }
    free(buffer);
    return result;
}

#endif
