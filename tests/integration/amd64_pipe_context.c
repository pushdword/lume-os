/* SPDX-License-Identifier: BSD-3-Clause */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "lume/abi.h"
#include "lume/tty.h"

static const uint64_t pattern[4] = {UINT64_C(0x0123456789abcdef), UINT64_C(0xfedcba9876543210),
                                    UINT64_C(0xa55a112233445566), UINT64_C(0x1234abcd6789ef01)};

struct pipe_vector_state
{
    uint64_t vector[4];
    uint32_t original, selected, observed;
};

_Static_assert(offsetof(struct pipe_vector_state, original) == 32, "original MXCSR offset");
_Static_assert(offsetof(struct pipe_vector_state, selected) == 36, "selected MXCSR offset");
_Static_assert(offsetof(struct pipe_vector_state, observed) == 40, "observed MXCSR offset");

static int64_t transfer(unsigned number, int fd, void *address, uint64_t bytes)
{
    struct pipe_vector_state state = {.selected = 0x3f80};
    register uint64_t operation __asm__("r8") = number;
    uint64_t result = (unsigned)fd;
    __asm__ volatile("stmxcsr 32(%[state])\n\t"
                     "ldmxcsr 36(%[state])\n\t"
                     "movdqu 0(%[pattern]), %%xmm0\n\t"
                     "movdqu 16(%[pattern]), %%xmm15\n\t"
                     "syscall\n\t"
                     "movdqu %%xmm0, 0(%[state])\n\t"
                     "movdqu %%xmm15, 16(%[state])\n\t"
                     "stmxcsr 40(%[state])\n\t"
                     "ldmxcsr 32(%[state])"
                     : "+D"(result), "+m"(state)
                     : "S"(address), "d"(bytes),
                       "r"(operation), [state] "r"(&state), [pattern] "r"(pattern)
                     : "rax", "rcx", "r11", "xmm0", "xmm15", "cc", "memory");
    if (memcmp(state.vector, pattern, sizeof(pattern)) || state.observed != state.selected)
        return -1;
    return (int64_t)result;
}

int main(void)
{
    int descriptors[2];
    if (pipe(descriptors))
        return 1;
    for (unsigned iteration = 0; iteration < 32; ++iteration)
    {
        unsigned char source = (unsigned char)iteration, target = 0;
        if (transfer(LUME_SYS_WRITE, descriptors[1], &source, 1) != 1 ||
            transfer(LUME_SYS_READ, descriptors[0], &target, 1) != 1 || source != target)
            return 2;
        if (transfer(LUME_SYS_READ, descriptors[0], &target, 0) ||
            transfer(LUME_SYS_WRITE, descriptors[1], &source, 0))
            return 3;
        struct lume_tty_request request = {
            .version = LUME_TTY_VERSION, .bytes = sizeof(request), .fd = descriptors[0]};
        struct lume_tty_request before = request;
        if (transfer(LUME_SYS_TTY, LUME_TTY_GET, &request, sizeof(request)) != -25 ||
            memcmp(&request, &before, sizeof(request)))
            return 4;
    }
    if (close(descriptors[0]) || close(descriptors[1]))
        return 5;
    puts("PASS_AMD64_PIPE_CONTEXT");
    return 0;
}
