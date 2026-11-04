/* SPDX-License-Identifier: BSD-3-Clause */
#define HAVE_MORECORE 1
#define HAVE_MMAP 0
#define HAVE_MREMAP 0
#define MORECORE_CANNOT_TRIM 1
#define USE_LOCKS 1
#define USE_SPIN_LOCKS 0
#define USE_RECURSIVE_LOCKS 0
#define MALLOC_ALIGNMENT 16
#define STRUCT_MALLINFO_DECLARED 1
#include <malloc.h>
#include "../../vendor/libffi/src/dlmalloc.c"

void cfree(void *pointer)
{
    free(pointer);
}

void *aligned_alloc(size_t alignment, size_t bytes)
{
    if (!alignment || (alignment & (alignment - 1)) || bytes % alignment)
    {
        errno = EINVAL;
        return NULL;
    }
    return memalign(alignment, bytes);
}
