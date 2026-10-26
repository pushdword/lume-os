.section .text.start,"ax"
.global _start
_start:
    xorq %rbp, %rbp
    movq (%rsp), %rdi
    leaq 8(%rsp), %rsi
    andq $-16, %rsp
    call main
    movslq %eax, %rdi
    movq $1, %r8
    int $0x80
    ud2
.section .note.GNU-stack,"",@progbits
