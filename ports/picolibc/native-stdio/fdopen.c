/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2019 Keith Packard
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "streams.h"
#include "lume/abi.h"
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

static size_t stream_buffer_size(int fd, bool writable)
{
    bool use_metadata = writable;
#if defined(__FSTAT_BUFSIZ) && defined(_STAT_HAS_ST_BLKSIZE)
    use_metadata = true;
#endif
    struct stat metadata;
    if (use_metadata && !fstat(fd, &metadata))
    {
        if (writable && S_ISREG(metadata.st_mode))
            return LUME_FILE_IO_MAX;
#if defined(__FSTAT_BUFSIZ) && defined(_STAT_HAS_ST_BLKSIZE)
        if (metadata.st_blksize > 0)
            return (size_t)metadata.st_blksize;
#endif
    }
    return BUFSIZ;
}

FILE *
fdopen(int fd, const char *mode)
{
    int                  stdio_flags;
    int                  open_flags;
    struct __file_bufio *bf;
    char                *buf;
    size_t               buf_size;

    stdio_flags = __stdio_flags(mode, &open_flags);
    if (stdio_flags == 0)
        return NULL;

    buf_size = stream_buffer_size(fd, (stdio_flags & __SWR) != 0);

    /* Allocate file structure and necessary buffers */
    struct lume_stdio_bufio *stream = calloc(1, sizeof(*stream) + buf_size);
    if (!stream && buf_size > BUFSIZ)
    {
        buf_size = BUFSIZ;
        stream = calloc(1, sizeof(*stream) + buf_size);
    }
    if (stream == NULL)
        return NULL;

    bf = &stream->buffer;
    buf = (char *)(stream + 1);

    *bf = (struct __file_bufio)FDEV_SETUP_BUFIO(fd, buf, buf_size, read, write, lseek, close,
                                               stdio_flags, __BFALL);
    bf->xfile.cfile.close = lume_stdio_close_bufio;
    bf->xfile.setvbuf = lume_stdio_setvbuf;

    if (open_flags & O_APPEND)
        (void)fseeko(&(bf->xfile.cfile.file), 0, SEEK_END);

    stream->stream.buffered = true;
    lume_stdio_register(&stream->stream, &bf->xfile.cfile.file);
    return &bf->xfile.cfile.file;
}
