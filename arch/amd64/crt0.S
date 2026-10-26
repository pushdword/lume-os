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
#endif
    movq (%r12), %rdi
    leaq 8(%r12), %rsi
    leaq 16(%r12,%rdi,8), %rdx
    call app_main
#ifdef LUME_NATIVE_CXX
    movl %eax, %edi
    call exit
#else
    movl %eax, %r13d
    call fflush_all_streams
    movl %r13d, %edi
    call app_exit
#endif
    ud2
.section .note.GNU-stack,"",@progbits
