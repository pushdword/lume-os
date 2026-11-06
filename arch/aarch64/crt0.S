.section .text.start,"ax"
.global _start
_start:
    mov x29, xzr
    mov x30, xzr
    bl lume_tls_init
    ldr x19, [sp]
    add x20, sp, #8
    add x21, x20, x19, lsl #3
    add x21, x21, #8
    mov x0, x19
    mov x1, x20
    mov x2, x21
    bl lume_runtime_init
#ifdef LUME_NATIVE_CXX
    bl native_cxx_initialize
#else
    bl lume_runtime_c_initialize
#endif
    mov x0, x19
    mov x1, x20
    mov x2, x21
    bl app_main
    bl exit
.section .note.GNU-stack,"",%progbits
