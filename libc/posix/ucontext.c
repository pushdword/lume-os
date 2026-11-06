/* SPDX-License-Identifier: BSD-3-Clause */
#if defined(LUME_ARCH_AARCH64)
#include <ucontext.h>
#include "lume/context_offsets.h"
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define CONTEXT_VALID UINT64_C(0x4c554d4543545831)
#define CONTEXT_FPCR UINT32_C(0x07c89f00)
#define CONTEXT_FPSR UINT32_C(0x0800009f)

_Static_assert(sizeof(void *) == 8 && sizeof(sigset_t) == 4, "context data model");
_Static_assert(offsetof(ucontext_t, uc_mcontext) == LUME_CONTEXT_MACHINE, "context layout");
_Static_assert(offsetof(mcontext_t, sp) == LUME_CONTEXT_SP, "context stack layout");
_Static_assert(offsetof(mcontext_t, pc) == LUME_CONTEXT_PC, "context continuation layout");
_Static_assert(offsetof(mcontext_t, arguments) == LUME_CONTEXT_ARGUMENTS, "context arguments");
_Static_assert(offsetof(mcontext_t, floating) == LUME_CONTEXT_FLOATING, "context FP registers");
_Static_assert(offsetof(mcontext_t, fpcr) == LUME_CONTEXT_FPCR, "context FP control");
_Static_assert(offsetof(mcontext_t, fpsr) == LUME_CONTEXT_FPSR, "context FP status");
_Static_assert(offsetof(mcontext_t, validity) == LUME_CONTEXT_VALIDITY, "context validity");

extern void lume_context_return(void);

int lume_context_invalid(void)
{
    errno = EINVAL;
    return -1;
}

static int valid_context(const ucontext_t *context)
{
    if (!context || context->uc_flags || context->reserved ||
        context->uc_mcontext.validity != CONTEXT_VALID || !context->uc_mcontext.sp ||
        (context->uc_mcontext.sp & 15) || !context->uc_mcontext.pc ||
        (context->uc_mcontext.pc & 3) || (context->uc_mcontext.fpcr & ~CONTEXT_FPCR) ||
        (context->uc_mcontext.fpsr & ~CONTEXT_FPSR))
        return lume_context_invalid();
    return 0;
}

int lume_context_capture(ucontext_t *context)
{
    context->uc_mcontext.validity = 0;
    if (sigprocmask(SIG_SETMASK, NULL, &context->uc_sigmask))
        return -1;
    context->uc_flags = context->reserved = 0;
    context->uc_mcontext.validity = CONTEXT_VALID;
    return 0;
}

int lume_context_activate(const ucontext_t *context)
{
    return valid_context(context) ? -1 : sigprocmask(SIG_SETMASK, &context->uc_sigmask, NULL);
}

int lume_context_exchange(ucontext_t *outgoing, const ucontext_t *incoming)
{
    outgoing->uc_mcontext.validity = 0;
    if (outgoing == incoming)
        return lume_context_invalid();
    if (valid_context(incoming) ||
        sigprocmask(SIG_SETMASK, &incoming->uc_sigmask, &outgoing->uc_sigmask))
        return -1;
    outgoing->uc_flags = outgoing->reserved = 0;
    outgoing->uc_mcontext.validity = CONTEXT_VALID;
    return 0;
}

void makecontext(ucontext_t *context, void (*function)(void), int count, ...)
{
    if (!context)
    {
        (void)lume_context_invalid();
        return;
    }
    uint64_t validity = context->uc_mcontext.validity;
    context->uc_mcontext.validity = 0;
    context->uc_mcontext.pc = 0;
    uintptr_t bottom = (uintptr_t)context->uc_stack.ss_sp;
    size_t bytes = context->uc_stack.ss_size;
    if (validity != CONTEXT_VALID || !function || ((uintptr_t)function & 3) || count < 0 ||
        context->uc_flags || context->reserved || context->uc_stack.ss_flags || !bottom ||
        bytes > UINTPTR_MAX - bottom || bytes < LUME_UCONTEXT_STACK_MIN)
    {
        (void)lume_context_invalid();
        return;
    }
    uintptr_t top = (bottom + bytes) & ~(uintptr_t)15;
    size_t stack_count = count > 8 ? (size_t)count - 8 : 0;
    if (top - bottom < LUME_UCONTEXT_STACK_MIN ||
        stack_count > (top - bottom) / sizeof(uintptr_t) ||
        stack_count * sizeof(uintptr_t) > top - bottom - LUME_UCONTEXT_STACK_MIN)
    {
        (void)lume_context_invalid();
        return;
    }
    uintptr_t stack = (top - stack_count * sizeof(uintptr_t)) & ~(uintptr_t)15;
    if (stack < bottom || stack - bottom < LUME_UCONTEXT_STACK_MIN)
    {
        (void)lume_context_invalid();
        return;
    }
    memset(context->uc_mcontext.registers, 0, sizeof(context->uc_mcontext.registers));
    memset(context->uc_mcontext.arguments, 0, sizeof(context->uc_mcontext.arguments));
    va_list arguments;
    va_start(arguments, count);
    for (int index = 0; index < count; ++index)
    {
        uintptr_t value = va_arg(arguments, uintptr_t);
        if (index < 8)
            context->uc_mcontext.arguments[index] = value;
        else
            ((uintptr_t *)stack)[index - 8] = value;
    }
    va_end(arguments);
    context->uc_mcontext.registers[0] = (uintptr_t)context;
    context->uc_mcontext.registers[11] = (uintptr_t)lume_context_return;
    context->uc_mcontext.sp = stack;
    context->uc_mcontext.pc = (uintptr_t)function;
    context->uc_mcontext.validity = CONTEXT_VALID;
}

_Noreturn void lume_context_finish(ucontext_t *context)
{
    if (!context->uc_link)
        exit(0);
    (void)setcontext(context->uc_link);
    abort();
}
#endif
