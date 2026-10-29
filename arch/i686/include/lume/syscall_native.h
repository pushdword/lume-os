#ifndef LUME_ARCH_SYSCALL_NATIVE_H
#define LUME_ARCH_SYSCALL_NATIVE_H
#include <stdint.h>
#include "lume/target.h"

/* EAX addresses the request. EDX:EAX returns the signed 64-bit result. */
static inline int64_t lume_native_call(uint64_t number, uint64_t a, uint64_t b, uint64_t c)
{
    const uint64_t request[4] = {number, a, b, c};
    uint32_t lo = (uintptr_t)request, hi;
    __asm__ volatile("int $0x80" : "+a"(lo), "=d"(hi) : : "memory", "cc");
    return (int64_t)((uint64_t)hi << 32 | lo);
}
#endif
