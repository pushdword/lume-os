.section .text.start,"ax"
.global _start
_start:
    mov x29, xzr
    mov x30, xzr
    ldr x0, [sp]
    add x1, sp, #8
    bl main
    mov x8, #1
    svc #0x4c55
1:  b 1b
.section .note.GNU-stack,"",%progbits
