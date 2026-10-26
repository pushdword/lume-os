.section .text.boot,"ax"
.balign 8
.global _start
_start:
    b primary
    nop
    .quad 0
    .quad __image_end - _start
    .quad 8
    .quad 0, 0, 0
    .long 0x644d5241
    .long 0
primary:
    msr daifset, #15
    mov x20, x0
    mrs x1, mpidr_el1
    mov x2, #0xffffff
    movk x2, #0xff, lsl #32
    tst x1, x2
    b.ne stopped
    mrs x1, CurrentEL
    cmp x1, #4
    b.eq el1
    cmp x1, #8
    b.ne stopped
    mrs x1, sctlr_el2
    tbnz x1, #0, stopped
    tbnz x1, #2, stopped
    /* The UART monitor runs at the firmware's EL, without changing MMU state. */
    mrs x1, cnthctl_el2
    orr x1, x1, #3
    msr cnthctl_el2, x1
    msr cntvoff_el2, xzr
    b relocate
el1:
    mrs x1, sctlr_el1
    tbnz x1, #0, stopped
    tbnz x1, #2, stopped
relocate:
    /* Firmware may place an Image at 2 MiB despite its configured address.
     * Move the file before using absolute symbols or the C runtime. */
    adr x1, _start
    ldr x2, =__image_start
    cmp x1, x2
    b.eq stack
    mov x3, #0x200000
    cmp x1, x3
    b.ne stopped
    mov x3, #0x800000
    cmp x20, x3
    b.hs 3f
    mov x3, #0x400000
    cmp x20, x3
    b.hs stopped
3:  ldr x3, =__load_end - _start
4:  ldrb w4, [x1], #1
    strb w4, [x2], #1
    subs x3, x3, #1
    b.ne 4b
    dsb sy
    ic iallu
    dsb sy
    isb
    ldr x1, =stack
    br x1
stack:
    msr spsel, #1
    ldr x1, =__stack_top
    mov sp, x1
    ldr x1, =__bss_start
    ldr x2, =__bss_end
1:  cmp x1, x2
    b.hs 2f
    stp xzr, xzr, [x1], #16
    b 1b
2:  mov x0, x20
    bl lrec_main
stopped:
    wfe
    b stopped
.global lrec_jump
.type lrec_jump,%function
lrec_jump:
    msr daifset, #15
    mov x4, #0x200000
    mov x1, #0
    mov x2, #0
    mov x3, #0
    br x4
.section .note.GNU-stack,"",%progbits
