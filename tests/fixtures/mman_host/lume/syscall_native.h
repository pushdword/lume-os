#ifndef LUME_MMAN_HOST_SYSCALL_H
#define LUME_MMAN_HOST_SYSCALL_H
#include <stdint.h>
int64_t lume_mman_host_call(uint64_t, uint64_t, uint64_t, uint64_t);
#define lume_native_call lume_mman_host_call
#endif
