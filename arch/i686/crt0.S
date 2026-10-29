.section .text.start,"ax"
.global _start
_start:
    xorl %ebp, %ebp
    movl %esp, %esi
    andl $-16, %esp
    call lume_tls_init
    movl (%esi), %ebx
    leal 4(%esi), %ecx
    leal 8(%esi,%ebx,4), %edx
    subl $4, %esp
    pushl %edx
    pushl %ecx
    pushl %ebx
    call lume_runtime_init
    call app_main
    addl $16, %esp
    movl %eax, %ebx
    call fflush_all_streams
    subl $12, %esp
    pushl %ebx
    call app_exit
    ud2
.section .note.GNU-stack,"",@progbits
