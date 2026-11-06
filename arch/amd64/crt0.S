.section .text.start,"ax"
.global _start
_start:
    xorq %rbp, %rbp
    movq %rsp, %r12
    andq $-16, %rsp
    call lume_tls_init
    movq (%r12), %rdi
    leaq 8(%r12), %rsi
    leaq 16(%r12,%rdi,8), %rdx
    call lume_runtime_init
#ifdef LUME_NATIVE_CXX
    call native_cxx_initialize
#else
    call lume_runtime_c_initialize
#endif
    movq (%r12), %rdi
    leaq 8(%r12), %rsi
    leaq 16(%r12,%rdi,8), %rdx
    call app_main
    movl %eax, %edi
    call exit
    ud2
.section .note.GNU-stack,"",@progbits
