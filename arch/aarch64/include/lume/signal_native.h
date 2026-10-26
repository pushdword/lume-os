#ifndef LUME_ARCH_SIGNAL_NATIVE_H
#define LUME_ARCH_SIGNAL_NATIVE_H

__attribute__((naked, noreturn)) static void signal_restorer(void)
{
    __asm__ volatile("mov x0, #4\nmov x1, #0\nmov x2, #0\nmov x8, #48\nsvc #0x4c55\nbrk #0");
}
#endif
