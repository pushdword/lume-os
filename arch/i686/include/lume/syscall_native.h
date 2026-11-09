#ifndef LUME_ARCH_SYSCALL_NATIVE_H
#define LUME_ARCH_SYSCALL_NATIVE_H
#include <stdint.h>
#include "lume/target.h"
#include "lume/syscall_numbers.h"

/* INT 0x80 passes the request through EAX. INT 0x81 takes GETPID in EAX.
 * EDX:EAX returns the signed 64-bit result for both entry points. */
static inline int64_t lume_native_call(uint64_t number, uint64_t a, uint64_t b, uint64_t c)
{
    if (number == LUME_SYS_GETPID)
    {
        uint32_t lo = LUME_SYS_GETPID, hi;
        __asm__ volatile("int $0x81" : "+a"(lo), "=d"(hi) : : "memory", "cc");
        return (int64_t)((uint64_t)hi << 32 | lo);
    }
    const uint64_t request[4] = {number, a, b, c};
    uint32_t lo = (uintptr_t)request, hi;
    __asm__ volatile("int $0x80" : "+a"(lo), "=d"(hi) : : "memory", "cc");
    return (int64_t)((uint64_t)hi << 32 | lo);
}
#endif
