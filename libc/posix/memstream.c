/* SPDX-License-Identifier: BSD-3-Clause */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>

struct memory_stream
{
    char **output;
    size_t *size;
    char *buffer;
    size_t position, length, capacity;
};

static void publish(struct memory_stream *stream)
{
    *stream->output = stream->buffer;
    *stream->size = stream->position < stream->length ? stream->position : stream->length;
}

static ssize_t memory_write(void *cookie, const void *data, size_t bytes)
{
    struct memory_stream *stream = cookie;
    if (bytes > (size_t)SSIZE_MAX - stream->position)
    {
        errno = EFBIG;
        return -1;
    }
    size_t end = stream->position + bytes;
    if (end >= stream->capacity)
    {
        size_t capacity = stream->capacity <= (size_t)SSIZE_MAX / 2 ? stream->capacity * 2
                                                                    : (size_t)SSIZE_MAX + 1;
        if (capacity <= end)
            capacity = end + 1;
        char *buffer = realloc(stream->buffer, capacity);
        if (!buffer)
            return -1;
        stream->buffer = buffer;
        stream->capacity = capacity;
    }
    if (stream->position > stream->length)
        memset(stream->buffer + stream->length, 0, stream->position - stream->length);
    memcpy(stream->buffer + stream->position, data, bytes);
    stream->position = end;
    if (end > stream->length)
        stream->length = end;
    stream->buffer[stream->length] = 0;
    publish(stream);
    return (ssize_t)bytes;
}

static off_t memory_seek(void *cookie, off_t offset, int whence)
{
    struct memory_stream *stream = cookie;
    if (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END)
    {
        errno = EINVAL;
        return -1;
    }
    off_t base = whence == SEEK_CUR   ? (off_t)stream->position
                 : whence == SEEK_END ? (off_t)stream->length
                                      : 0;
    if (offset < -base || offset > SSIZE_MAX - base)
    {
        errno = EINVAL;
        return -1;
    }
    stream->position = (size_t)(base + offset);
    publish(stream);
    return (off_t)stream->position;
}

static int memory_close(void *cookie)
{
    struct memory_stream *stream = cookie;
    publish(stream);
    free(stream);
    return 0;
}

FILE *open_memstream(char **output, size_t *size)
{
    if (!output || !size)
    {
        errno = EINVAL;
        return NULL;
    }
    struct memory_stream *stream = calloc(1, sizeof(*stream));
    if (!stream)
        return NULL;
    stream->buffer = calloc(1, 1);
    if (!stream->buffer)
    {
        free(stream);
        return NULL;
    }
    stream->output = output;
    stream->size = size;
    stream->capacity = 1;
    FILE *file = funopen(stream, NULL, memory_write, memory_seek, memory_close);
    if (!file)
    {
        free(stream->buffer);
        free(stream);
        return NULL;
    }
    publish(stream);
    return file;
}
