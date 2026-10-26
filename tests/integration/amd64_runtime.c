#include "lume/abi.h"
#include "lume/syscall_native.h"
#include "lume/signal.h"
#include "lume/signal_native.h"
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static _Thread_local uint64_t tls_initialized = UINT64_C(0x123456789abcdef0);
static _Thread_local unsigned tls_zero;
static volatile unsigned signaled;

static int floating_context(uint64_t identity)
{
    uint64_t input[2] = {identity, ~identity}, output[2];
    unsigned short original, expected, actual;
    unsigned char extended[10];
    __asm__ volatile("fnstcw %0" : "=m"(original));
    expected = (unsigned short)((original & ~0xc00u) | (identity & 1 ? 0x400 : 0x800));
    __asm__ volatile("fldcw %0\nfld1\nmovdqu %1, %%xmm7" : : "m"(expected), "m"(input) : "xmm7");
    int64_t result = lume_native_call(LUME_SYS_SLEEP, 25, 0, 0);
    __asm__ volatile("movdqu %%xmm7, %0\nfnstcw %1\nfstpt %2\nfldcw %3"
                     : "=m"(output), "=m"(actual), "=m"(extended)
                     : "m"(original)
                     : "xmm7");
    static const unsigned char one[10] = {0, 0, 0, 0, 0, 0, 0, 0x80, 0xff, 0x3f};
    return !result && actual == expected && !memcmp(input, output, sizeof(input)) &&
           !memcmp(extended, one, sizeof(one));
}

static void *thread_entry(void *argument)
{
    uintptr_t identity = (uintptr_t)argument;
    if (tls_initialized != UINT64_C(0x123456789abcdef0) || tls_zero)
        return (void *)1;
    tls_initialized = identity;
    for (unsigned i = 0; i < 4; ++i)
    {
        ++tls_zero;
        if (!floating_context(identity) || tls_initialized != identity)
            return (void *)2;
    }
    return tls_zero == 4 ? (void *)(identity + 100) : (void *)3;
}

static void signal_handler(int number)
{
    signaled = (unsigned)number;
}

int app_main(void)
{
    puts("AMD64_RUNTIME_BEGIN");
    if (lume_native_call(LUME_SYS_ABI, 0, 0, 0) != LUME_ABI_VERSION ||
        lume_native_call(LUME_SYS_PROBE, 0x200000, 1, 0) != -LUME_EFAULT ||
        lume_native_call(LUME_SYS_PROBE, 0xf0001010, 1, 1) != -LUME_EFAULT)
        return 10;
    tls_initialized = 77;
    tls_zero = 19;
    pthread_t threads[2];
    if (pthread_create(&threads[0], NULL, thread_entry, (void *)1) ||
        pthread_create(&threads[1], NULL, thread_entry, (void *)2))
        return 11;
    for (unsigned i = 0; i < 4; ++i)
        if (!floating_context(3) || tls_initialized != 77 || tls_zero != 19)
            return 12;
    for (unsigned i = 0; i < 2; ++i)
    {
        void *value;
        if (pthread_join(threads[i], &value) || value != (void *)(uintptr_t)(101 + i))
            return 13;
    }
    puts("AMD64_TLS_X87_SSE_THREADS_PASS");
    struct lume_signal_request action = {.version = LUME_SIGNAL_VERSION,
                                         .bytes = sizeof(action),
                                         .number = 10,
                                         .operation = 1,
                                         .handler = (uintptr_t)signal_handler,
                                         .restorer = (uintptr_t)signal_restorer};
    if (lume_native_call(LUME_SYS_SIGNAL, LUME_SIGNAL_ACTION, (uintptr_t)&action, sizeof(action)))
        return 14;
    struct lume_signal_request send = {.version = LUME_SIGNAL_VERSION,
                                       .bytes = sizeof(send),
                                       .number = 10,
                                       .pid = (int32_t)lume_native_call(LUME_SYS_GETPID, 0, 0, 0)};
    if (lume_native_call(LUME_SYS_SIGNAL, LUME_SIGNAL_SEND, (uintptr_t)&send, sizeof(send)) ||
        signaled != 10 || tls_initialized != 77)
        return 15;
    puts("AMD64_SIGNAL_RETURN_PASS");
    int64_t child = lume_native_call(LUME_SYS_FORK, 0, 0, 0);
    if (child < 0)
        return 16;
    if (!child)
    {
        unsigned char *data = malloc(4096);
        if (!data)
            return 17;
        data[0] = 0xc3;
        ((void (*)(void))(uintptr_t)data)();
        return 18;
    }
    int64_t status = lume_native_call(LUME_SYS_WAIT, (uint64_t)child, 1, 1);
    if (status < 0 || (uint64_t)status >> 32 != (uint64_t)child || (uint32_t)status != 11)
        return 19;
    puts("AMD64_FORK_NX_ISOLATION_PASS");
    puts("AMD64_RUNTIME_PASS");
    return 0;
}
