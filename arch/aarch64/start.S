.section .text.start,"ax"
.global _start
_start:
    mov x29, xzr
    mov x30, xzr
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
#endif
    mov x0, x19
    mov x1, x20
    mov x2, x21
    bl app_main
#ifdef LUME_NATIVE_CXX
    bl exit
#else
    mov x19, x0
    bl fflush_all_streams
    mov x0, x19
    bl app_exit
#endif
.section .note.GNU-stack,"",%progbits
