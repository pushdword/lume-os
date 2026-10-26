#include <stddef.h>
#include <stdint.h>

extern unsigned char __tls_start[], __tls_file_end[], __tls_size[], __tls_align[];
static _Alignas(64) unsigned char initial_tls[65536 + 64];

void lume_tls_init(void)
{
    size_t align = (uintptr_t)__tls_align;
    size_t bias = (16 + align - 1) & ~(align - 1);
    for (size_t i = 0; i < (size_t)(__tls_file_end - __tls_start); ++i)
        initial_tls[bias + i] = __tls_start[i];
    __asm__ volatile("msr tpidr_el0, %0; isb" ::"r"(initial_tls) : "memory");
}
