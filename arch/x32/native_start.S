.section .text.entry,"ax",@progbits
.globl _start
_start:
    cld
    push EDI
    push ESI
    push EDX
    call lume_runtime_init
    call native_cxx_initialize
    pop EDX
    pop ESI
    pop EDI
    call app_main
    mov EDI,EAX
    call exit
.globl lume_syscall
lume_syscall:
    push EBX
    mov EAX,EDI
    mov EBX,ESI
    mov E10,EDX
    mov EDX,ECX
    mov ECX,E10
    int 128
    pop EBX
    ret
