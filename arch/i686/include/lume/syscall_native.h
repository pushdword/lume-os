#ifndef LUME_ARCH_SYSCALL_NATIVE_H
#define LUME_ARCH_SYSCALL_NATIVE_H
#include <stdint.h>
#include "lume/target.h"
#include "lume/syscall_numbers.h"

/* INT 0x80 passes the request through EAX. INT 0x81 takes simple call numbers
 * in EAX and 32-bit arguments in EBX, ECX and EDX. Both return EDX:EAX. */
static inline int64_t lume_native_call(uint64_t number, uint64_t a, uint64_t b, uint64_t c)
{
    if (number == LUME_SYS_GETPID)
    {
        uint32_t lo = LUME_SYS_GETPID, hi;
        __asm__ volatile("int $0x81" : "+a"(lo), "=d"(hi) : : "memory", "cc");
        return (int64_t)((uint64_t)hi << 32 | lo);
    }
    if ((number == LUME_SYS_READ || number == LUME_SYS_WRITE) && (a | b | c) <= UINT32_MAX)
    {
        uint32_t lo = (uint32_t)number, hi = (uint32_t)c;
        __asm__ volatile("int $0x81"
                         : "+a"(lo), "+d"(hi)
                         : "b"((uint32_t)a), "c"((uint32_t)b)
                         : "memory", "cc");
        int64_t result = (int64_t)((uint64_t)hi << 32 | lo);
        /* Older kernels report ENOSYS for direct read and write requests. */
        if (result != -38)
            return result;
    }
    const uint64_t request[4] = {number, a, b, c};
    uint32_t lo = (uintptr_t)request, hi;
    __asm__ volatile("int $0x80" : "+a"(lo), "=d"(hi) : : "memory", "cc");
    return (int64_t)((uint64_t)hi << 32 | lo);
}
#endif
