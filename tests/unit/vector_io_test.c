#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ssize_t ptrdiff_t

struct iovec
{
    void *iov_base;
    size_t iov_len;
};

static unsigned allocations, releases, copies, calls;
static size_t transferred, limit = SIZE_MAX;
static void *last_pointer;
static int io_error;
static bool allocation_error;
static unsigned char temporary[65536], captured[65536];

static void *test_malloc(size_t bytes)
{
    assert(bytes && bytes <= sizeof(temporary));
    ++allocations;
    if (allocation_error)
    {
        errno = ENOMEM;
        return NULL;
    }
    return temporary;
}

static void test_free(void *pointer)
{
    assert(pointer == temporary);
    ++releases;
}

static void *test_memcpy(void *out, const void *in, size_t bytes)
{
    copies += (unsigned)bytes;
    return memcpy(out, in, bytes);
}

static ssize_t test_write(int fd, const void *data, size_t bytes)
{
    assert(fd == 7 && bytes <= sizeof(captured));
    ++calls;
    last_pointer = (void *)data;
    transferred = bytes;
    if (io_error)
    {
        errno = io_error;
        return -1;
    }
    size_t result = bytes < limit ? bytes : limit;
    memcpy(captured, data, result);
    return (ssize_t)result;
}

static ssize_t test_read(int fd, void *data, size_t bytes)
{
    assert(fd == 7 && bytes <= sizeof(captured));
    ++calls;
    last_pointer = data;
    transferred = bytes;
    if (io_error)
    {
        errno = io_error;
        return -1;
    }
    size_t result = bytes < limit ? bytes : limit;
    memset(data, 'r', result);
    return (ssize_t)result;
}

#define malloc test_malloc
#define free test_free
#define memcpy test_memcpy
#define write test_write
#define read test_read
#include "../../libc/posix/vector_io.h"
#undef malloc
#undef free
#undef memcpy
#undef write
#undef read

static void reset(void)
{
    allocations = releases = copies = calls = 0;
    transferred = 0;
    last_pointer = NULL;
    io_error = 0;
    allocation_error = false;
    limit = SIZE_MAX;
}

int main(void)
{
    unsigned char payload[65537];
    memset(payload, 'x', sizeof(payload));
    struct iovec vectors[3] = {{NULL, 0}, {payload, 65536}, {NULL, 0}};
    for (unsigned writing = 0; writing < 2; ++writing)
    {
        reset();
        assert(vector_io(7, vectors, 3, writing) == 65536);
        assert(calls == 1 && last_pointer == payload && transferred == 65536);
        assert(!allocations && !releases && !copies);
        reset();
        io_error = EAGAIN;
        assert(vector_io(7, vectors + 1, 1, writing) == -1 && errno == EAGAIN);
        assert(calls == 1 && !allocations && !releases && !copies);
        reset();
        limit = 17;
        assert(vector_io(7, vectors, 3, writing) == 17 && calls == 1 && !copies);
        reset();
        vectors[1].iov_len = sizeof(payload);
        assert(vector_io(7, vectors, 3, writing) == 65536 && transferred == 65536);
        vectors[1].iov_len = 65536;
    }
    reset();
    assert(!vector_io(7, NULL, 0, 1) && !calls && !allocations);
    assert(vector_io(7, vectors, -1, 1) == -1 && errno == EINVAL);
    assert(vector_io(7, vectors, 1025, 1) == -1 && errno == EINVAL);
    vectors[2] = (struct iovec){payload, SIZE_MAX / 2};
    assert(vector_io(7, vectors, 3, 1) == -1 && errno == EINVAL && !calls);
    vectors[0] = (struct iovec){payload, 8};
    vectors[1] = (struct iovec){payload + 8, 16};
    vectors[2] = (struct iovec){NULL, 0};
    memset(payload, 'x', sizeof(payload));
    assert(vector_io(7, vectors, 3, 1) == 24);
    assert(calls == 1 && allocations == 1 && releases == 1 && copies == 24);
    assert(!memcmp(captured, payload, 24));
    reset();
    limit = 11;
    assert(vector_io(7, vectors, 3, 0) == 11);
    assert(calls == 1 && allocations == 1 && releases == 1 && copies == 11);
    assert(payload[10] == 'r' && payload[11] == 'x');
    reset();
    io_error = EAGAIN;
    assert(vector_io(7, vectors, 3, 0) == -1 && errno == EAGAIN);
    assert(calls == 1 && allocations == 1 && releases == 1 && !copies);
    reset();
    allocation_error = true;
    assert(vector_io(7, vectors, 3, 1) == -1 && errno == ENOMEM);
    assert(!calls && allocations == 1 && !releases);
    puts("PASS vector I/O direct buffers, bounds, partial transfers and errors");
    return 0;
}
