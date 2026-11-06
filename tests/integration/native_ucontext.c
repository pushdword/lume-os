/* SPDX-License-Identifier: BSD-3-Clause */
#include <ucontext.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static ucontext_t caller, child;
static _Alignas(16) unsigned char stack[32768];
static unsigned state;
static unsigned capture_stage;
static uint64_t child_fpcr, child_fpsr, child_d8;

static uint64_t d8(void)
{
    uint64_t value;
    __asm__ volatile("fmov %0, d8" : "=r"(value));
    return value;
}

static uint64_t fpcr(void)
{
    uint64_t value;
    __asm__ volatile("mrs %0, fpcr" : "=r"(value));
    return value;
}

static uint64_t fpsr(void)
{
    uint64_t value;
    __asm__ volatile("mrs %0, fpsr" : "=r"(value));
    return value;
}

static void entry(int low, int high)
{
    if ((uint32_t)low != UINT32_C(0x76543210) || (uint32_t)high != UINT32_C(0xfedcba98))
        abort();
    uint64_t control = (fpcr() & ~UINT64_C(0xc00000)) | UINT64_C(0x400000);
    uint64_t status = UINT64_C(0x10);
    uint64_t vector = UINT64_C(0x7fe0123456789abc);
    __asm__ volatile("fmov d8, %0" ::"r"(vector) : "d8");
    __asm__ volatile("msr fpcr, %0\nmsr fpsr, %1" ::"r"(control), "r"(status) : "memory");
    child_fpcr = fpcr();
    child_fpsr = fpsr();
    child_d8 = d8();
    sigset_t mask;
    if (sigprocmask(SIG_SETMASK, NULL, &mask) || !(mask & (UINT32_C(1) << SIGUSR1)))
        abort();
    state = 1;
    if (swapcontext(&child, &caller) || state != 2 || fpcr() != child_fpcr ||
        fpsr() != child_fpsr || d8() != child_d8)
        abort();
    state = 3;
}

static void words(uintptr_t a, uintptr_t b, uintptr_t c, uintptr_t d, uintptr_t e, uintptr_t f,
                  uintptr_t g, uintptr_t h, uintptr_t i, uintptr_t j, uintptr_t k, uintptr_t l)
{
    if (a != 1 || b != 2 || c != 3 || d != 4 || e != 5 || f != 6 || g != 7 || h != 8 ||
        i != UINT64_C(0x100000009) || j != 10 || k != 11 || l != 12)
        abort();
    state = 4;
}

static int probe(void)
{
    ucontext_t saved;
    capture_stage = 0;
    if (getcontext(&saved))
        return 5;
    if (!capture_stage)
    {
        capture_stage = 1;
        if (setcontext(&saved))
            return 6;
        return 7;
    }
    uint64_t original_fpcr = fpcr(), original_fpsr = fpsr();
    uint64_t vector = UINT64_C(0x3fe076543210abcd);
    __asm__ volatile("fmov d8, %0" ::"r"(vector) : "d8");
    uint64_t original_d8 = d8();
    sigset_t original_mask;
    if (getcontext(&child))
        return 1;
    child.uc_stack = (stack_t){.ss_sp = stack, .ss_size = sizeof(stack)};
    child.uc_link = &caller;
    child.uc_sigmask |= UINT32_C(1) << SIGUSR1;
    makecontext(&child, (void (*)(void))entry, 2, (int)UINT32_C(0x76543210),
                (int)UINT32_C(0xfedcba98));
    if (sigprocmask(SIG_SETMASK, NULL, &original_mask) || swapcontext(&caller, &child) ||
        state != 1 || fpcr() != original_fpcr || fpsr() != original_fpsr || d8() != original_d8)
        return 2;
    sigset_t returned_mask;
    if (sigprocmask(SIG_SETMASK, NULL, &returned_mask) || returned_mask != original_mask)
        return 3;
    state = 2;
    if (swapcontext(&caller, &child) || state != 3)
        return 4;
    if (getcontext(&child))
        return 8;
    child.uc_stack = (stack_t){.ss_sp = stack, .ss_size = sizeof(stack)};
    child.uc_link = &caller;
    makecontext(&child, (void (*)(void))words, 12, (uintptr_t)1, (uintptr_t)2, (uintptr_t)3,
                (uintptr_t)4, (uintptr_t)5, (uintptr_t)6, (uintptr_t)7, (uintptr_t)8,
                (uintptr_t)UINT64_C(0x100000009), (uintptr_t)10, (uintptr_t)11, (uintptr_t)12);
    if (swapcontext(&caller, &child) || state != 4)
        return 9;
    return 0;
}

static void *thread_probe(void *argument)
{
    (void)argument;
    return (void *)(uintptr_t)probe();
}

int main(void)
{
    int result = probe();
    if (result)
    {
        printf("NATIVE_UCONTEXT_FAIL stage=%d errno=%d\n", result, errno);
        return 1;
    }
    pthread_t thread;
    void *value;
    if (pthread_create(&thread, NULL, thread_probe, NULL) || pthread_join(thread, &value) || value)
        return 2;
    puts("NATIVE_UCONTEXT_PASS");
    return 0;
}
