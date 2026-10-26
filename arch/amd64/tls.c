#include "lume/abi.h"
#include "lume/syscall_native.h"
#include <stddef.h>
#include <stdint.h>
extern unsigned char __tls_start[], __tls_file_end[], __tls_size[], __tls_align[];
static _Alignas(64) unsigned char initial_tls[65536 + 128];

void lume_tls_init(void)
{
    size_t size = (uintptr_t)__tls_size;
    size_t align = (uintptr_t)__tls_align;
    if (size > 65536 || !align || align > 64 || (align & (align - 1)))
        __builtin_trap();
    size_t span = (size + align - 1) & ~(align - 1);
    for (size_t i = 0; i < (size_t)(__tls_file_end - __tls_start); ++i)
        initial_tls[i] = __tls_start[i];
    uintptr_t tp = (uintptr_t)initial_tls + span;
    ((uintptr_t *)tp)[0] = tp;
    ((uintptr_t *)tp)[1] = 0;
    if (lume_native_call(LUME_SYS_TLS, tp, 0, 0))
        __builtin_trap();
}
