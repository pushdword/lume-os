/* SPDX-License-Identifier: BSD-3-Clause */
#include "streams.h"
#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

static bool pending_pushback(const FILE *file)
{
#ifdef __ATOMIC_UNGETC
    return __atomic_load_n(&file->unget, __ATOMIC_RELAXED) != 0;
#else
    return file->unget != 0;
#endif
}

static bool read_unbuffered(FILE *file, unsigned char *out, size_t bytes, size_t *received)
{
    if (!(file->flags & __SBUF) || file == stdin || bytes < 2)
        return false;

    struct __file_bufio *stream = (struct __file_bufio *)file;
    __bufio_lock(file);
    if (stream->size != 1 || stream->len != stream->off || (stream->bflags & __BFPTR) ||
        stream->read_int != read || pending_pushback(file))
    {
        __bufio_unlock(file);
        return false;
    }

    int fd = (int)(intptr_t)stream->ptr;
    struct stat metadata;
    int saved_errno = errno;
    bool regular = !fstat(fd, &metadata) && S_ISREG(metadata.st_mode);
    errno = saved_errno;
    if (!regular)
    {
        __bufio_unlock(file);
        return false;
    }

    if (__bufio_setdir_locked(file, __SRD) < 0)
        file->flags |= __SERR;
    else
    {
        stream->len = stream->off = 0;
        /* Read only the requested bytes directly into the caller's destination. */
        while (*received < bytes)
        {
            ssize_t n = read(fd, out + *received, bytes - *received);
            if (n <= 0)
            {
                file->flags |= n < 0 ? __SERR : __SEOF;
                break;
            }
            *received += (size_t)n;
            stream->pos += n;
        }
    }
    __bufio_unlock(file);
    return true;
}

static size_t read_items(unsigned char *out, size_t size, size_t count, FILE *file)
{
    if (!(file->flags & __SRD) || !size || !count)
        return 0;
    if (count > SIZE_MAX / size)
    {
        errno = EOVERFLOW;
        file->flags |= __SERR;
        return 0;
    }

    size_t bytes = size * count, received = 0;
    if (!read_unbuffered(file, out, bytes, &received))
    {
        while (received < bytes)
        {
            int c = getc_unlocked(file);
            if (c == EOF)
                break;
            out[received++] = (unsigned char)c;
        }
    }
    return received / size;
}

size_t fread(void *out, size_t size, size_t count, FILE *file)
{
#ifdef __STDIO_LOCKING
    flockfile(file);
#endif
    size_t result = read_items(out, size, count, file);
#ifdef __STDIO_LOCKING
    funlockfile(file);
#endif
    return result;
}
