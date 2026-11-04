/* SPDX-License-Identifier: BSD-3-Clause */
#include <assert.h>
#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/lock.h>
#include <time.h>

#ifdef LUME_PICOLIBC_ALLOCATOR
extern void *__malloc_free_list;
#endif
static void *blocks[20000];

static uint64_t milliseconds(void)
{
    struct timespec time;
    assert(!clock_gettime(CLOCK_MONOTONIC, &time));
    return (uint64_t)time.tv_sec * 1000 + time.tv_nsec / 1000000;
}

static void check_free_list(void)
{
#ifdef LUME_PICOLIBC_ALLOCATOR
    __LIBC_LOCK();
    uintptr_t previous = 0;
    unsigned count = 0;
    for (void **node = __malloc_free_list; node; node = *node)
    {
        assert((uintptr_t)node > previous && ++count <= 50000);
        previous = (uintptr_t)node;
    }
    __LIBC_UNLOCK();
#endif
}

static void *worker(void *opaque)
{
    unsigned number = (unsigned)(uintptr_t)opaque;
    for (unsigned pass = 0; pass < 4; ++pass)
        for (unsigned index = number; index < 20000; index += 4)
        {
            size_t before = 32 + index % 128;
            unsigned char byte = (unsigned char)index;
            unsigned char *old = blocks[index];
            for (size_t i = 0; i < before; ++i)
                assert(old[i] == byte);
            unsigned char *next = realloc(old, before + 256);
            assert(next && !((uintptr_t)next & 15));
            for (size_t i = 0; i < before; ++i)
                assert(next[i] == byte);
            free(next);
            blocks[index] = malloc(before);
            assert(blocks[index]);
            memset(blocks[index], byte, before);
        }
    return NULL;
}

int main(void)
{
    void *(*volatile allocate)(size_t) = malloc;
    void *(*volatile clear)(size_t, size_t) = calloc;
    void *(*volatile resize)(void *, size_t) = realloc;
    errno = 0;
    void *failed = allocate(SIZE_MAX);
    int allocation_error = errno;
    printf("ALLOCATOR_OVERFLOW pointer=%p errno=%d expected=%d\n", failed, allocation_error, ENOMEM);
    assert(!failed && allocation_error == ENOMEM);
    volatile size_t excessive = SIZE_MAX;
    assert(!clear(excessive, 2));
    unsigned char *zero = calloc(512, 1);
    assert(zero && malloc_usable_size(zero) >= 512);
    for (unsigned index = 0; index < 512; ++index)
        assert(!zero[index]);
    memset(zero, 0xa5, 512);
    assert(!resize(zero, excessive));
    for (unsigned index = 0; index < 512; ++index)
        assert(zero[index] == 0xa5);
    free(zero);
    void *aligned = NULL;
    assert(!posix_memalign(&aligned, 4096, 8192) && !((uintptr_t)aligned & 4095));
    free(aligned);
    aligned = aligned_alloc(64, 128);
    assert(aligned && !((uintptr_t)aligned & 63));
    cfree(aligned);
    aligned = malloc(0);
    free(aligned);
    free(NULL);
    struct mallinfo information = mallinfo();
    assert(information.arena >= information.uordblks);
    uint64_t start = milliseconds();
    for (unsigned index = 0; index < 20000; ++index)
    {
        blocks[index] = malloc(32 + index % 128);
        assert(blocks[index] && !((uintptr_t)blocks[index] & 15));
        memset(blocks[index], (unsigned char)index, 32 + index % 128);
    }
    check_free_list();
    for (unsigned index = 0; index < 20000; index += 2)
    {
        free(blocks[index]);
        blocks[index] = NULL;
    }
    check_free_list();
    uint64_t fragmented = milliseconds();
    void *large[2000];
    for (unsigned index = 0; index < 2000; ++index)
        assert((large[index] = malloc(512)));
    uint64_t allocated = milliseconds();
    for (unsigned index = 0; index < 2000; ++index)
        free(large[index]);
    for (unsigned index = 0; index < 20000; index += 2)
    {
        blocks[index] = malloc(32 + index % 128);
        assert(blocks[index]);
        memset(blocks[index], (unsigned char)index, 32 + index % 128);
    }
    check_free_list();
    pthread_t threads[4];
    for (unsigned index = 0; index < 4; ++index)
        assert(!pthread_create(&threads[index], NULL, worker, (void *)(uintptr_t)index));
    for (unsigned index = 0; index < 4; ++index)
        assert(!pthread_join(threads[index], NULL));
    check_free_list();
    for (unsigned index = 0; index < 20000; ++index)
        free(blocks[index]);
    check_free_list();
    printf(
        "ALLOCATOR_CONTENT_ALIGNMENT_CONCURRENCY_PASS setup_ms=%llu fragmented_ms=%llu total_ms=%llu\n",
        (unsigned long long)(fragmented - start), (unsigned long long)(allocated - fragmented),
        (unsigned long long)(milliseconds() - start));
    return 0;
}
