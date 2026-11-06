/* SPDX-License-Identifier: BSD-3-Clause */
#include <ucontext.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

extern int lume_context_capture(ucontext_t *);
extern int lume_context_activate(const ucontext_t *);
extern int lume_context_exchange(ucontext_t *, const ucontext_t *);
extern int lume_context_invalid(void);
static sigset_t current_mask;
static int mask_error;
static unsigned mask_calls;

int sigprocmask(int operation, const sigset_t *set, sigset_t *old)
{
    assert(operation == SIG_SETMASK);
    ++mask_calls;
    if (mask_error)
    {
        errno = mask_error;
        return -1;
    }
    sigset_t previous = current_mask;
    if (set)
        current_mask = *set;
    if (old)
        *old = previous;
    return 0;
}

void lume_context_return(void)
{
    assert(0);
}

int setcontext(const ucontext_t *context)
{
    (void)context;
    assert(0);
    return -1;
}

static void entry(void)
{
}

static void prepare(ucontext_t *context, void *stack, size_t bytes)
{
    memset(context, 0, sizeof(*context));
    context->uc_mcontext.pc = (uintptr_t)entry;
    context->uc_mcontext.sp = (uintptr_t)stack + bytes;
    assert(lume_context_capture(context) == 0);
    context->uc_stack = (stack_t){.ss_sp = stack, .ss_size = bytes};
}

int main(void)
{
    static _Alignas(16) unsigned char stack[8192];
    ucontext_t context, link;
    prepare(&context, stack, sizeof(stack));
    context.uc_link = &link;
    current_mask = UINT32_C(1) << SIGUSR1;
    assert(lume_context_capture(&context) == 0 && context.uc_sigmask == current_mask);
    makecontext(&context, entry, 12, (uintptr_t)1, (uintptr_t)2, (uintptr_t)3, (uintptr_t)4,
                (uintptr_t)5, (uintptr_t)6, (uintptr_t)7, (uintptr_t)8,
                (uintptr_t)UINT64_C(0x100000009), (uintptr_t)10, (uintptr_t)11, (uintptr_t)12);
    assert(context.uc_link == &link && context.uc_mcontext.pc == (uintptr_t)entry);
    assert(!(context.uc_mcontext.sp & 15) &&
           context.uc_mcontext.registers[0] == (uintptr_t)&context);
    assert(context.uc_mcontext.registers[11] == (uintptr_t)lume_context_return);
    for (unsigned i = 0; i < 8; ++i)
        assert(context.uc_mcontext.arguments[i] == i + 1);
    uintptr_t *arguments = (uintptr_t *)context.uc_mcontext.sp;
    assert(arguments[0] == UINT64_C(0x100000009) && arguments[1] == 10 && arguments[3] == 12);
    unsigned before = mask_calls;
    current_mask = UINT32_C(1) << SIGUSR2;
    assert(lume_context_exchange(&link, &context) == 0);
    assert(mask_calls == before + 1 && link.uc_sigmask == (UINT32_C(1) << SIGUSR2));
    assert(current_mask == context.uc_sigmask);
    mask_error = ENOSYS;
    assert(lume_context_capture(&link) == -1 && errno == ENOSYS && !link.uc_mcontext.validity);
    assert(lume_context_activate(&context) == -1 && errno == ENOSYS);
    mask_error = 0;
    context.uc_mcontext.fpcr = UINT32_C(0x80000000);
    assert(lume_context_activate(&context) == -1 && errno == EINVAL);
    context.uc_mcontext.fpcr = 0;
    context.uc_mcontext.fpsr = UINT32_C(0x80000000);
    assert(lume_context_activate(&context) == -1 && errno == EINVAL);
    prepare(&context, stack, sizeof(stack));
    makecontext(&context, entry, -1);
    assert(errno == EINVAL && !context.uc_mcontext.pc && !context.uc_mcontext.validity);
    assert(lume_context_activate(&context) == -1 && errno == EINVAL);
    prepare(&context, stack, sizeof(stack));
    context.uc_stack.ss_size = 32;
    makecontext(&context, entry, 0);
    assert(errno == EINVAL && !context.uc_mcontext.validity);
    prepare(&context, stack, sizeof(stack));
    context.uc_stack.ss_sp = (void *)(UINTPTR_MAX - 31);
    makecontext(&context, entry, 0);
    assert(errno == EINVAL && !context.uc_mcontext.validity);
    prepare(&context, stack, sizeof(stack));
    makecontext(&context, entry, INT_MAX);
    assert(errno == EINVAL && !context.uc_mcontext.validity);
    prepare(&context, stack, sizeof(stack));
    makecontext(&context, entry, 0);
    assert(context.uc_mcontext.validity && lume_context_activate(&context) == 0);
    assert(lume_context_invalid() == -1 && errno == EINVAL);
    puts("PASS_UCONTEXT_LAYOUT_AND_MASKS");
    return 0;
}
