/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_COREUTILS_STDIO_SUPPORT_H
#define LUME_COREUTILS_STDIO_SUPPORT_H

#include <stddef.h>
#include <stdio-bufio.h>

static inline size_t lume_coreutils_read_ahead(FILE *stream)
{
    size_t bytes = stream->unget != 0;
    if (stream->flags & __SBUF)
    {
        struct __file_bufio *buffer = (struct __file_bufio *)stream;
        if (buffer->dir == __SRD && buffer->len > buffer->off)
        {
            bytes += (size_t)(buffer->len - buffer->off);
        }
    }
    return bytes;
}

static inline size_t lume_coreutils_write_pending(FILE *stream)
{
    if (stream->flags & __SBUF)
    {
        struct __file_bufio *buffer = (struct __file_bufio *)stream;
        if (buffer->dir == __SWR && buffer->len > 0)
        {
            return (size_t)buffer->len;
        }
    }
    return 0;
}

static inline int lume_coreutils_purge(FILE *stream)
{
    if (stream->flags & __SBUF)
    {
        struct __file_bufio *buffer = (struct __file_bufio *)stream;
        buffer->len = 0;
        buffer->off = 0;
    }
    stream->unget = 0;
    return 0;
}

static inline int lume_coreutils_reading(FILE *stream)
{
    if (!(stream->flags & __SRD))
    {
        return 0;
    }
    if (!(stream->flags & __SWR))
    {
        return 1;
    }
    return (stream->flags & __SBUF) && ((struct __file_bufio *)stream)->dir == __SRD;
}

#endif
