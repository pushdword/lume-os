.section .text.start,"ax"
.global _start
_start:
    xorl %ebp, %ebp
    movl (%esp), %eax
    leal 4(%esp), %ecx
    leal 8(%esp,%eax,4), %edx
    andl $-16, %esp
    subl $4, %esp
    pushl %edx
    pushl %ecx
    pushl %eax
    call main
    addl $16, %esp
    subl $32, %esp
    movl $1, (%esp)
    movl $0, 4(%esp)
    movl %eax, 8(%esp)
    movl $0, 12(%esp)
    movl $0, 16(%esp)
    movl $0, 20(%esp)
    movl $0, 24(%esp)
    movl $0, 28(%esp)
    movl %esp, %eax
    int $0x80
    ud2
.section .note.GNU-stack,"",@progbits
