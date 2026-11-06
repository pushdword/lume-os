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
#ifdef LUME_NATIVE_CXX
    call native_cxx_initialize
#else
    call lume_runtime_c_initialize
#endif
    call app_main
    addl $16, %esp
    subl $12, %esp
    pushl %eax
    call exit
    ud2
.section .note.GNU-stack,"",@progbits
