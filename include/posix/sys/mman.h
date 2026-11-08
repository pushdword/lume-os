#ifndef LUME_MMAN_H
#define LUME_MMAN_H
#include <sys/types.h>
#define PROT_NONE 0
#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_EXEC 4
#define MAP_SHARED 1
#define MAP_PRIVATE 2
#define MAP_FIXED 16
#define MAP_NORESERVE 0x4000
#define MAP_FIXED_NOREPLACE 0x100000
#define MAP_JIT 0x200000
#define MAP_ANON 32
#define MAP_ANONYMOUS MAP_ANON
#define MAP_FAILED ((void *)-1)
#define MADV_NORMAL 0
#define MADV_RANDOM 1
#define MADV_SEQUENTIAL 2
#define MADV_WILLNEED 3
#define MADV_DONTNEED 4
#define MADV_FREE 8
#define POSIX_MADV_NORMAL MADV_NORMAL
#define POSIX_MADV_RANDOM MADV_RANDOM
#define POSIX_MADV_SEQUENTIAL MADV_SEQUENTIAL
#define POSIX_MADV_WILLNEED MADV_WILLNEED
#define POSIX_MADV_DONTNEED MADV_DONTNEED
#define MFD_CLOEXEC 1u
#define MFD_ALLOW_SEALING 2u
#ifdef __cplusplus
extern "C"
{
#endif
    void *mmap(void *, size_t, int, int, int, off_t);
    int munmap(void *, size_t);
    int mprotect(void *, size_t, int);
    void *lume_mmap_aligned(void *, size_t, int, int, int, off_t, size_t);
    int madvise(void *, size_t, int);
    int lume_clear_cache(void *, size_t);
    int lume_jit_write_begin(void);
    int lume_jit_write_end(void);
    /* POSIX advice preserves contents and errno. Valid advice with zero length succeeds
     * without accessing mappings. Nonempty ranges require VM alignment and mapped coverage. */
    int posix_madvise(void *, size_t, int);
    int memfd_create(const char *, unsigned);
    int shm_open(const char *, int, mode_t);
    int shm_unlink(const char *);
    int lume_shm_duplicate(int, int, unsigned);
    int mlock(const void *, size_t);
    int munlock(const void *, size_t);
#ifdef __cplusplus
}
#endif
#endif
