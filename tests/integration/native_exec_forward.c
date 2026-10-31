/* SPDX-License-Identifier: BSD-3-Clause */
#include "lume/abi.h"
#include "lume/process.h"
#include "lume/syscall_native.h"
#include <stdint.h>

#ifndef LUME_FORWARD_TARGET
#error LUME_FORWARD_TARGET is required
#endif

__asm__(".section .text.start,\"ax\"\n"
        ".global _start\n"
        "_start:\n"
        "ldr x0, [sp]\n"
        "add x1, sp, #8\n"
        "add x2, x1, x0, lsl #3\n"
        "add x2, x2, #8\n"
        "b lume_forward_entry\n");

__attribute__((noreturn, used)) void lume_forward_entry(uint64_t argc, char **argv, char **envp)
{
    struct lume_run_request request = {.path = (uintptr_t)LUME_FORWARD_TARGET,
                                       .argv = (uintptr_t)argv,
                                       .envp = (uintptr_t)envp,
                                       .argc = (uint32_t)argc,
                                       .envc = 0};
    while (request.envc < LUME_ENV_MAX && envp[request.envc])
        ++request.envc;
    if (argc && argc < LUME_ARG_MAX && request.envc < LUME_ENV_MAX)
        lume_native_call(LUME_SYS_EXEC, (uintptr_t)&request, sizeof(request), 0);
    lume_native_call(LUME_SYS_EXIT, 127, 0, 0);
    for (;;)
    {
    }
}
