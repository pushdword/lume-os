/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_STDIO_STREAMS_H
#define LUME_STDIO_STREAMS_H

#include <stdio-bufio.h>
#include <stdbool.h>

struct lume_stdio_stream
{
    FILE *file;
    struct lume_stdio_stream *next;
    unsigned pins;
    bool buffered, output_active;
};

struct lume_stdio_bufio
{
    struct __file_bufio buffer;
    struct lume_stdio_stream stream;
};

void lume_stdio_register(struct lume_stdio_stream *, FILE *);
void lume_stdio_unregister(struct lume_stdio_stream *);
int lume_stdio_close_bufio(FILE *);
int lume_stdio_setvbuf(FILE *, char *, int, size_t);
int __stdio_flags(const char *, int *);

#endif
