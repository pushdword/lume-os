/* SPDX-License-Identifier: BSD-3-Clause */
#include "streams.h"
#include <errno.h>
#include <pthread.h>

struct flush_cursor
{
    struct flush_cursor *previous, *next;
    struct lume_stdio_stream *pending, *pinned;
};

static pthread_mutex_t stream_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t unpinned = PTHREAD_COND_INITIALIZER;
static struct lume_stdio_stream *streams;
static struct flush_cursor *flushes;

void lume_stdio_register(struct lume_stdio_stream *stream, FILE *file)
{
    pthread_mutex_lock(&stream_lock);
    stream->file = file;
    stream->next = streams;
    streams = stream;
    pthread_mutex_unlock(&stream_lock);
}

void lume_stdio_unregister(struct lume_stdio_stream *stream)
{
    pthread_mutex_lock(&stream_lock);
    struct lume_stdio_stream **link = &streams;
    while (*link != stream)
        link = &(*link)->next;
    *link = stream->next;
    for (struct flush_cursor *cursor = flushes; cursor; cursor = cursor->next)
        if (cursor->pending == stream)
            cursor->pending = stream->next;
    while (stream->pins)
        pthread_cond_wait(&unpinned, &stream_lock);
    pthread_mutex_unlock(&stream_lock);
}

int lume_stdio_close_bufio(FILE *file)
{
    struct lume_stdio_bufio *stream = (struct lume_stdio_bufio *)file;
    lume_stdio_unregister(&stream->stream);
    return __bufio_close(file);
}

int lume_stdio_setvbuf(FILE *file, char *buffer, int mode, size_t size)
{
    int result = __bufio_setvbuf(file, buffer, mode, size);
    /* Picolibc selects its default close callback when it allocates a buffer. */
    ((struct __file_bufio *)file)->xfile.cfile.close = lume_stdio_close_bufio;
    return result;
}

static int flush_stream(FILE *file, bool output_only, const struct lume_stdio_stream *stream)
{
#ifdef __STDIO_LOCKING
    flockfile(file);
#endif
    bool buffered = output_only && stream && stream->buffered;
    if (buffered)
        __bufio_lock(file);

    bool output_active = buffered ? ((struct __file_bufio *)file)->dir == __SWR
                                 : !stream || __atomic_load_n(&stream->output_active, __ATOMIC_RELAXED);
    int result = 0;
    if (!output_only || ((file->flags & __SWR) && output_active))
    {
        if (buffered)
            result = __bufio_flush_locked(file);
        else if (file->flush)
            result = file->flush(file);
        if (output_only && result)
            file->flags |= __SERR;
#ifdef __ATOMIC_UNGETC
        __atomic_store_n(&file->unget, 0, __ATOMIC_RELAXED);
#else
        file->unget = 0;
#endif
    }
    if (buffered)
        __bufio_unlock(file);
#ifdef __STDIO_LOCKING
    funlockfile(file);
#endif
    return result;
}

static void unpin_cursor(struct flush_cursor *cursor)
{
    if (cursor->pinned && --cursor->pinned->pins == 0)
        pthread_cond_broadcast(&unpinned);
    cursor->pinned = NULL;
}

static void close_cursor(void *argument)
{
    struct flush_cursor *cursor = argument;
    pthread_mutex_lock(&stream_lock);
    unpin_cursor(cursor);
    if (cursor->previous)
        cursor->previous->next = cursor->next;
    else
        flushes = cursor->next;
    if (cursor->next)
        cursor->next->previous = cursor->previous;
    pthread_mutex_unlock(&stream_lock);
}

static void flush_output(FILE *file, const struct lume_stdio_stream *stream, int *result, int *error)
{
    if (flush_stream(file, true, stream) && !*result)
    {
        *result = EOF;
        *error = errno;
    }
}

int fflush(FILE *file)
{
    if (file)
        return flush_stream(file, false, NULL);

    int result = 0, error = errno;
    flush_output(stdout, NULL, &result, &error);
    if (stderr != stdout)
        flush_output(stderr, NULL, &result, &error);

    pthread_mutex_lock(&stream_lock);
    struct flush_cursor cursor = {.next = flushes, .pending = streams};
    if (flushes)
        flushes->previous = &cursor;
    flushes = &cursor;
    pthread_mutex_unlock(&stream_lock);

    /* Pin one stream at a time so other streams can close during a callback. */
    pthread_cleanup_push(close_cursor, &cursor);
    for (;;)
    {
        pthread_mutex_lock(&stream_lock);
        unpin_cursor(&cursor);
        struct lume_stdio_stream *stream = cursor.pending;
        if (stream)
        {
            cursor.pending = stream->next;
            cursor.pinned = stream;
            ++stream->pins;
        }
        pthread_mutex_unlock(&stream_lock);
        if (!stream)
            break;
        flush_output(stream->file, stream, &result, &error);
    }
    pthread_cleanup_pop(1);

    errno = error;
    return result;
}
