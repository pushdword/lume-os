#ifndef LUME_ARCH_SYSCALL_NATIVE_H
#define LUME_ARCH_SYSCALL_NATIVE_H
#include <stdint.h>
#include "lume/target.h"

/* Native SYSCALL requires the architectural SYSCALL feature. RCX and R11
 * are clobbered by entry. The result replaces RDI. */
static inline int64_t lume_native_call(uint64_t number, uint64_t a, uint64_t b, uint64_t c)
{
    register uint64_t r8 __asm__("r8") = number;
#if defined(LUME_NATIVE_INT80)
    __asm__ volatile("int $0x80" : "+D"(a) : "S"(b), "d"(c), "r"(r8) : "memory", "cc");
#else
    __asm__ volatile("syscall" : "+D"(a) : "S"(b), "d"(c), "r"(r8) : "rcx", "r11", "memory", "cc");
#endif
    return (int64_t)a;
}
#endif
