/* SPDX-License-Identifier: BSD-3-Clause */
#include "lume/abi.h"
#include "lume/process.h"
#include "lume/syscall_native.h"
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

static _Thread_local unsigned local_value = 17;
static int channels[2][2];
static unsigned simd;

__attribute__((target("sse3"))) static int vector_roundtrip(unsigned seed)
{
    _Alignas(16) uint32_t expected[32], actual[32];
    for (unsigned i = 0; i < 32; ++i)
        expected[i] = seed + i * 0x10001u;
    uint32_t control = 0x3f80, restored;
    const uint64_t request[4] = {LUME_SYS_SLEEP, 1, 0, 0};
    uint32_t call = (uintptr_t)request, high;
    __asm__ volatile("movups 0(%3), %%xmm0\nmovups 16(%3), %%xmm1\n"
                     "movups 32(%3), %%xmm2\nmovups 48(%3), %%xmm3\n"
                     "movups 64(%3), %%xmm4\nmovups 80(%3), %%xmm5\n"
                     "movups 96(%3), %%xmm6\nmovups 112(%3), %%xmm7\nldmxcsr %5\n"
                     "int $0x80\n"
                     "movups %%xmm0, 0(%4)\nmovups %%xmm1, 16(%4)\n"
                     "movups %%xmm2, 32(%4)\nmovups %%xmm3, 48(%4)\n"
                     "movups %%xmm4, 64(%4)\nmovups %%xmm5, 80(%4)\n"
                     "movups %%xmm6, 96(%4)\nmovups %%xmm7, 112(%4)\nstmxcsr %2"
                     : "+a"(call), "=d"(high), "=m"(restored)
                     : "S"(expected), "D"(actual), "m"(control)
                     : "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7", "memory",
                       "cc");
    control = 0x1f80;
    __asm__ volatile("ldmxcsr %0" : : "m"(control) : "memory");
    return restored == 0x3f80 && !memcmp(expected, actual, sizeof(expected));
}

static void *worker(void *argument)
{
    unsigned slot = (uintptr_t)argument;
    if (local_value != 17)
        return (void *)1;
    local_value = 90 + slot;
    for (unsigned i = 0; i < 8; ++i)
    {
        if (simd && !vector_roundtrip(slot * 1000 + i))
            return (void *)2;
        if (local_value != 90 + slot)
            return (void *)3;
    }
    unsigned char value = (unsigned char)local_value;
    if (write(channels[slot][1], &value, 1) != 1)
        return (void *)4;
    return NULL;
}

static int jit_memory(void)
{
    unsigned char *code = mmap(NULL, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                               MAP_PRIVATE | MAP_ANONYMOUS | MAP_JIT, -1, 0);
    if (code == MAP_FAILED || lume_jit_write_begin() || lume_jit_write_begin())
        return 1;
    const unsigned char instructions[] = {0xb8, 42, 0, 0, 0, 0xc3};
    memcpy(code, instructions, sizeof(instructions));
    if (lume_jit_write_end() || lume_jit_write_end() || ((int (*)(void))code)() != 42)
        return 2;
    errno = 0;
    if (lume_jit_write_end() != -1 || errno != EINVAL)
        return 3;
    pid_t child = fork();
    if (child < 0)
        return 4;
    if (!child)
        _exit(((int (*)(void))code)() == 42 ? 0 : 1);
    int status;
    if (waitpid(child, &status, 0) != child || !WIFEXITED(status) || WEXITSTATUS(status))
        return 5;
    return munmap(code, 4096) ? 6 : 0;
}

int main(void)
{
    puts("I686_NATIVE_BEGIN");
    struct lume_system_features features;
    if (lume_native_call(LUME_SYS_FEATURES, (uintptr_t)&features, sizeof(features),
                         LUME_FEATURE_VERSION))
        return 1;
    simd = !!(features.features & LUME_FEATURE_SIMD_CONTEXT);
    printf("I686_NATIVE_FEATURES SIMD=%u\n", simd);
    pthread_t threads[2];
    for (unsigned i = 0; i < 2; ++i)
        if (pipe(channels[i]) || pthread_create(&threads[i], NULL, worker, (void *)(uintptr_t)i))
            return 2;
    for (unsigned i = 0; i < 2; ++i)
    {
        unsigned char value;
        void *result;
        int joined = pthread_join(threads[i], &result);
        printf("I686_NATIVE_JOIN %u error=%d result=%u\n", i, joined, (unsigned)(uintptr_t)result);
        if (joined || result || read(channels[i][0], &value, 1) != 1 || value != 90 + i)
            return 3;
        close(channels[i][0]);
        close(channels[i][1]);
    }
    if (local_value != 17)
        return 4;
    puts("I686_NATIVE_JIT_BEGIN");
    int jit = jit_memory();
    if (jit)
    {
        printf("FAIL_NATIVE_I686_JIT %d errno %d\n", jit, errno);
        return 5;
    }
    printf("PASS_NATIVE_I686_RUNTIME threads, TLS, pipes, JIT, fork and SIMD=%u\n", simd);
    return 0;
}
