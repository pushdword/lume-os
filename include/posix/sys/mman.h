#ifndef LUME_MMAN_H
#define LUME_MMAN_H
#include <sys/types.h>
#define PROT_NONE 0
#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_EXEC 4
#define MAP_PRIVATE 2
#define MAP_ANON 32
#define MAP_ANONYMOUS MAP_ANON
#define MAP_FAILED ((void *)-1)
void *mmap(void *, size_t, int, int, int, off_t);
int munmap(void *, size_t);
int mprotect(void *, size_t, int);
int mlock(const void *, size_t);
int munlock(const void *, size_t);
#endif
