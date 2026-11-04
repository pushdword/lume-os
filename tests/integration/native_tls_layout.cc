/* SPDX-License-Identifier: BSD-3-Clause */
extern "C" void _start(void);

volatile unsigned tls_constructor_state;
alignas(64) thread_local unsigned char tls_zero_storage[257];
#if TLS_INITIALIZED
thread_local unsigned tls_counter = 17;
#else
thread_local unsigned tls_counter;
#endif

struct Initializer
{
    Initializer()
    {
        tls_constructor_state = 42;
    }
};

Initializer tls_initializer;

__attribute__((destructor)) static void tls_finalize(void)
{
    tls_constructor_state = 0;
}

void *volatile tls_got_anchor __attribute__((section(".got"))) = reinterpret_cast<void *>(&_start);

extern "C" __attribute__((section(".text.start"))) void _start(void)
{
    for (;;)
    {
        tls_constructor_state = tls_counter + tls_zero_storage[256] + (tls_got_anchor != nullptr);
    }
}
