/* SPDX-License-Identifier: BSD-3-Clause */
#define main lume_vm_fixture_main
#include "vm_test.c"
#undef main

#define mmap lume_test_mmap
#define munmap lume_test_munmap
#define mprotect lume_test_mprotect
#define madvise lume_test_madvise
#define posix_madvise lume_test_posix_madvise
#define memfd_create lume_test_memfd_create
#define shm_open lume_test_shm_open
#define shm_unlink lume_test_shm_unlink
#define mlock lume_test_mlock
#define munlock lume_test_munlock
#include "../../libc/posix/mman.c"

int lume_errno_from_syscall(unsigned error)
{
    switch (error)
    {
    case 12:
        return ENOMEM;
    case 22:
        return EINVAL;
    case 95:
        return ENOTSUP;
    default:
        fprintf(stderr, "Unexpected VM error %u\n", error);
        abort();
    }
}

static unsigned advice_calls;
static unsigned advice_seen;

int64_t lume_mman_host_call(uint64_t number, uint64_t operation, uint64_t address, uint64_t bytes)
{
    CHECK(number == LUME_SYS_VM && bytes == sizeof(struct lume_vm_request));
    struct lume_vm_request *request = (void *)(uintptr_t)address;
    CHECK(request->version == LUME_VM_VERSION && operation == LUME_VM_ADVISE);
    ++advice_calls;
    advice_seen = request->advice;
    return lume_vm_call(&files, 1, (unsigned)operation, request);
}

static void patterns(uint64_t base, bool zero)
{
    for (unsigned n = 0; n < 3; ++n)
    {
        unsigned char *page = (void *)(uintptr_t)physical(0, base + n * 4096, true, false);
        for (unsigned i = 0; i < 4096; ++i)
            CHECK(page[i] == (zero ? 0 : (unsigned char)(0xa7 + n)));
    }
}

int main(void)
{
    uint64_t base = call(0, LUME_VM_MAP, 0, 3 * 4096, LUME_PROT_READ | LUME_PROT_WRITE,
                         LUME_MAP_PRIVATE | LUME_MAP_ANONYMOUS, -1);
    CHECK(base == LUME_VM_BASE);
    for (unsigned n = 0; n < 3; ++n)
        memset((void *)(uintptr_t)physical(0, base + n * 4096, true, false), 0xa7 + n, 4096);
    unsigned frames_before = physical_live, metadata_before = metadata_live;
    const int advices[] = {POSIX_MADV_NORMAL, POSIX_MADV_RANDOM, POSIX_MADV_SEQUENTIAL,
                           POSIX_MADV_WILLNEED, POSIX_MADV_DONTNEED};
    for (unsigned i = 0; i < sizeof(advices) / sizeof(advices[0]); ++i)
    {
        errno = EBUSY;
        CHECK(posix_madvise((void *)(uintptr_t)base, 3 * 4096, advices[i]) == 0);
        CHECK(errno == EBUSY);
        patterns(base, false);
        CHECK(physical_live == frames_before && metadata_live == metadata_before);
    }
    CHECK(advice_seen == MADV_NORMAL);
    unsigned before = advice_calls;
    errno = ERANGE;
    CHECK(posix_madvise((void *)(uintptr_t)base, 4096, MADV_FREE) == EINVAL);
    CHECK(posix_madvise((void *)(uintptr_t)base, 4096, -1) == EINVAL);
    CHECK(posix_madvise((void *)(uintptr_t)base, 4096, 99) == EINVAL);
    CHECK(errno == ERANGE && advice_calls == before);
    CHECK(posix_madvise((void *)(uintptr_t)(base + 1), 0, POSIX_MADV_DONTNEED) == 0);
    CHECK(errno == ERANGE && advice_calls == before);
    CHECK(posix_madvise((void *)(uintptr_t)(base + 1), 4096, POSIX_MADV_DONTNEED) == EINVAL);
    CHECK(errno == ERANGE);
    CHECK(posix_madvise((void *)(uintptr_t)(base + 3 * 4096), 4096, POSIX_MADV_DONTNEED) == ENOMEM);
    CHECK(errno == ERANGE);
    CHECK(posix_madvise((void *)(uintptr_t)base, 4 * 4096, POSIX_MADV_DONTNEED) == ENOMEM);
    CHECK(errno == ERANGE);
    CHECK(posix_madvise((void *)(uintptr_t)(UINTPTR_MAX & ~(uintptr_t)4095), SIZE_MAX,
                        POSIX_MADV_NORMAL) == EINVAL);
    CHECK(errno == ERANGE);
    patterns(base, false);
    CHECK(madvise((void *)(uintptr_t)base, 3 * 4096, MADV_DONTNEED) == 0);
    patterns(base, true);
    clean();
    printf("PASS_POSIX_MADVISE checks=%u advice_calls=%u\n", checks, advice_calls);
    return 0;
}
