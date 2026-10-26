#ifndef LUME_ARCH_SIGNAL_NATIVE_H
#define LUME_ARCH_SIGNAL_NATIVE_H

__attribute__((naked, noreturn)) static void signal_restorer(void)
{
    __asm__ volatile("movq $4, %rdi\nxorq %rsi, %rsi\nxorq %rdx, %rdx\n"
                     "movq $48, %r8\nint $0x80\nud2");
}
#endif
