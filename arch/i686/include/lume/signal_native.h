#ifndef LUME_ARCH_SIGNAL_NATIVE_H
#define LUME_ARCH_SIGNAL_NATIVE_H

__attribute__((naked, noreturn)) static void signal_restorer(void)
{
    __asm__ volatile("subl $32, %esp\nmovl $48, (%esp)\nmovl $0, 4(%esp)\n"
                     "movl $4, 8(%esp)\nmovl $0, 12(%esp)\nmovl $0, 16(%esp)\n"
                     "movl $0, 20(%esp)\nmovl $0, 24(%esp)\nmovl $0, 28(%esp)\n"
                     "movl %esp, %eax\nint $0x80\nud2");
}
#endif
