#ifndef LUME_ARCH_SYSCALL_NATIVE_H
#define LUME_ARCH_SYSCALL_NATIVE_H
#include <stdint.h>
#include "lume/target.h"

/* INT 0x80 preserves all general-purpose registers except the result in RDI. */
static inline int64_t lume_native_call(uint64_t number, uint64_t a, uint64_t b, uint64_t c)
{
    register uint64_t r8 __asm__("r8") = number;
    __asm__ volatile("int $0x80" : "+D"(a) : "S"(b), "d"(c), "r"(r8) : "memory", "cc");
    return (int64_t)a;
}
#endif
