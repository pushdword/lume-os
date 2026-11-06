/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_POSIX_UCONTEXT_H
#define LUME_POSIX_UCONTEXT_H
#if !defined(LUME_ARCH_AARCH64) && !defined(__aarch64__)
#error "User contexts require AArch64"
#endif
#include <signal.h>
#include <stdint.h>

/* Contexts preserve the AArch64 function-call state and the signal mask.
 * They remain within one thread and do not represent signal-handler frames.
 * swapcontext requires distinct source and destination objects.
 * makecontext arguments occupy integer machine words. Floating-point values,
 * aggregates and scalable vector state are outside this interface. */
#define LUME_UCONTEXT_STACK_MIN 4096u

typedef struct
{
    uint64_t registers[12];
    uint64_t sp, pc;
    uint64_t arguments[8];
    uint64_t floating[8];
    uint32_t fpcr, fpsr;
    uint64_t validity;
} mcontext_t;

typedef struct ucontext_t
{
    uint64_t uc_flags;
    struct ucontext_t *uc_link;
    stack_t uc_stack;
    sigset_t uc_sigmask;
    uint32_t reserved;
    mcontext_t uc_mcontext;
} ucontext_t;

#ifdef __cplusplus
extern "C"
{
#endif
    int getcontext(ucontext_t *) __attribute__((returns_twice));
    int setcontext(const ucontext_t *);
    int swapcontext(ucontext_t *, const ucontext_t *) __attribute__((returns_twice));
    void makecontext(ucontext_t *, void (*)(void), int, ...);
#ifdef __cplusplus
}
#endif
#endif
