.section .text.entry,"ax",@progbits
.globl _start
_start:
mv r18,r10
mv r19,r11
mv r20,r12
call lume_runtime_init
mv r10,r18
mv r11,r19
mv r12,r20
call app_main
mv r18,r10
call fflush_all_streams
mv r10,r18
call app_exit
wfi
.globl lume_syscall
lume_syscall:
mv r17,r10
mv r10,r11
mv r11,r12
mv r12,r13
scall
ret
