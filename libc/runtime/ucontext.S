/* SPDX-License-Identifier: BSD-3-Clause */
#if defined(LUME_ARCH_AARCH64)
#include "lume/context_offsets.h"
.text
.p2align 2

.macro save_context
    add x9, x0, #LUME_CONTEXT_MACHINE
    stp x19, x20, [x9, #0]
    stp x21, x22, [x9, #16]
    stp x23, x24, [x9, #32]
    stp x25, x26, [x9, #48]
    stp x27, x28, [x9, #64]
    stp x29, x30, [x9, #80]
    mov x10, sp
    str x10, [x9, #LUME_CONTEXT_SP]
    str x30, [x9, #LUME_CONTEXT_PC]
    stp xzr, x1, [x9, #LUME_CONTEXT_ARGUMENTS]
    stp x2, x3, [x9, #128]
    stp x4, x5, [x9, #144]
    stp x6, x7, [x9, #160]
    stp d8, d9, [x9, #LUME_CONTEXT_FLOATING]
    stp d10, d11, [x9, #192]
    stp d12, d13, [x9, #208]
    stp d14, d15, [x9, #224]
    mrs x10, fpcr
    str w10, [x9, #LUME_CONTEXT_FPCR]
    mrs x10, fpsr
    str w10, [x9, #LUME_CONTEXT_FPSR]
.endm

.global getcontext
.type getcontext, %function
getcontext:
    cbz x0, .Linvalid
    save_context
    stp x29, x30, [sp, #-16]!
    mov x29, sp
    bl lume_context_capture
    ldp x29, x30, [sp], #16
    ret
.size getcontext, .-getcontext

.global swapcontext
.type swapcontext, %function
swapcontext:
    cbz x0, .Linvalid
    cbz x1, .Linvalid
    save_context
    stp x29, x30, [sp, #-32]!
    str x1, [sp, #16]
    mov x29, sp
    bl lume_context_exchange
    cbnz w0, .Lswap_failed
    ldr x0, [sp, #16]
    b .Lrestore
.Lswap_failed:
    ldp x29, x30, [sp], #32
    ret
.size swapcontext, .-swapcontext

.global setcontext
.type setcontext, %function
setcontext:
    cbz x0, .Linvalid
    stp x29, x30, [sp, #-32]!
    str x0, [sp, #16]
    mov x29, sp
    bl lume_context_activate
    cbnz w0, .Lset_failed
    ldr x0, [sp, #16]
    b .Lrestore
.Lset_failed:
    ldp x29, x30, [sp], #32
    ret
.size setcontext, .-setcontext

.Lrestore:
    add x9, x0, #LUME_CONTEXT_MACHINE
    ldr w10, [x9, #LUME_CONTEXT_FPCR]
    msr fpcr, x10
    ldr w10, [x9, #LUME_CONTEXT_FPSR]
    msr fpsr, x10
    ldp d8, d9, [x9, #LUME_CONTEXT_FLOATING]
    ldp d10, d11, [x9, #192]
    ldp d12, d13, [x9, #208]
    ldp d14, d15, [x9, #224]
    ldp x19, x20, [x9, #0]
    ldp x21, x22, [x9, #16]
    ldp x23, x24, [x9, #32]
    ldp x25, x26, [x9, #48]
    ldp x27, x28, [x9, #64]
    ldp x29, x30, [x9, #80]
    ldr x10, [x9, #LUME_CONTEXT_SP]
    ldr x16, [x9, #LUME_CONTEXT_PC]
    mov sp, x10
    ldp x0, x1, [x9, #LUME_CONTEXT_ARGUMENTS]
    ldp x2, x3, [x9, #128]
    ldp x4, x5, [x9, #144]
    ldp x6, x7, [x9, #160]
    br x16

.Linvalid:
    b lume_context_invalid

.global lume_context_return
.type lume_context_return, %function
lume_context_return:
    mov x0, x19
    b lume_context_finish
.size lume_context_return, .-lume_context_return
.section .note.GNU-stack,"",%progbits
#endif
