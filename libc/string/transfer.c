#include <stddef.h>
#include <stdint.h>
#include <limits.h>

typedef uintptr_t memory_word __attribute__((__may_alias__));

#if defined(__aarch64__) && defined(LUME_AARCH64_STRING_ASM)
extern void *__memcpy_aarch64_simd(void *, const void *, size_t);
extern void *__memmove_aarch64_simd(void *, const void *, size_t);
extern void *__memset_aarch64(void *, int, size_t);
#endif

#if defined(__clang__)
#define MEMORY_PRIMITIVE __attribute__((no_builtin("memcpy", "memset", "memmove")))
#elif defined(__GNUC__)
#define MEMORY_PRIMITIVE __attribute__((optimize("no-tree-loop-distribute-patterns")))
#else
#define MEMORY_PRIMITIVE
#endif

#if defined(__aarch64__) && !defined(LUME_AARCH64_STRING_ASM)
typedef uint64_t arm64_unaligned_word __attribute__((__may_alias__, __aligned__(1)));

static __attribute__((always_inline)) inline uint64_t arm64_load_word(const void *address)
{
#if defined(MEMORY_TEST_ARM64)
    return *(const volatile arm64_unaligned_word *)address;
#else
    uint64_t value;
    __asm__ volatile("ldr %0, [%1]" : "=r"(value) : "r"(address) : "memory");
    return value;
#endif
}

static __attribute__((always_inline)) inline void arm64_store_word(void *address, uint64_t value)
{
#if defined(MEMORY_TEST_ARM64)
    *(volatile arm64_unaligned_word *)address = value;
#else
    __asm__ volatile("str %0, [%1]" : : "r"(value), "r"(address) : "memory");
#endif
}

/* User buffers use Normal memory with unaligned accesses enabled. Every block
 * stays inside the requested range. Each overlapping copy loads 64 bytes before
 * storing them and advances in the direction that preserves unread bytes. */
static __attribute__((always_inline)) inline void *arm64_copy(void *destination, const void *source,
                                                              size_t count, unsigned backwards)
{
    unsigned char *out = destination;
    const unsigned char *in = source;
    size_t blocks = count / 128;
    if (backwards)
    {
        out += count;
        in += count;
    }
    if (blocks)
    {
#if defined(MEMORY_TEST_ARM64)
        typedef uint64_t unaligned_word __attribute__((__may_alias__, __aligned__(1)));
        do
        {
            for (unsigned half = 0; half < 2; ++half)
            {
                if (backwards)
                {
                    out -= 64;
                    in -= 64;
                }
                uint64_t values[8];
                for (unsigned i = 0; i < 8; ++i)
                    values[i] = ((const volatile unaligned_word *)in)[i];
                for (unsigned i = 0; i < 8; ++i)
                    ((volatile unaligned_word *)out)[i] = values[i];
                if (!backwards)
                {
                    out += 64;
                    in += 64;
                }
            }
        } while (--blocks);
#else
        if (backwards)
            __asm__ volatile("1:\n\t"
                             "ldp x3, x4, [%[in], #-16]\n\t"
                             "ldp x5, x6, [%[in], #-32]\n\t"
                             "ldp x7, x8, [%[in], #-48]\n\t"
                             "ldp x9, x10, [%[in], #-64]\n\t"
                             "stp x3, x4, [%[out], #-16]\n\t"
                             "stp x5, x6, [%[out], #-32]\n\t"
                             "stp x7, x8, [%[out], #-48]\n\t"
                             "stp x9, x10, [%[out], #-64]\n\t"
                             "ldp x3, x4, [%[in], #-80]\n\t"
                             "ldp x5, x6, [%[in], #-96]\n\t"
                             "ldp x7, x8, [%[in], #-112]\n\t"
                             "ldp x9, x10, [%[in], #-128]\n\t"
                             "stp x3, x4, [%[out], #-80]\n\t"
                             "stp x5, x6, [%[out], #-96]\n\t"
                             "stp x7, x8, [%[out], #-112]\n\t"
                             "stp x9, x10, [%[out], #-128]\n\t"
                             "sub %[in], %[in], #128\n\t"
                             "sub %[out], %[out], #128\n\t"
                             "subs %[blocks], %[blocks], #1\n\t"
                             "b.ne 1b"
                             : [out] "+&r"(out), [in] "+&r"(in), [blocks] "+&r"(blocks)
                             :
                             : "x3", "x4", "x5", "x6", "x7", "x8", "x9", "x10", "cc", "memory");
        else
            __asm__ volatile("1:\n\t"
                             "ldp x3, x4, [%[in]]\n\t"
                             "ldp x5, x6, [%[in], #16]\n\t"
                             "ldp x7, x8, [%[in], #32]\n\t"
                             "ldp x9, x10, [%[in], #48]\n\t"
                             "stp x3, x4, [%[out]]\n\t"
                             "stp x5, x6, [%[out], #16]\n\t"
                             "stp x7, x8, [%[out], #32]\n\t"
                             "stp x9, x10, [%[out], #48]\n\t"
                             "ldp x3, x4, [%[in], #64]\n\t"
                             "ldp x5, x6, [%[in], #80]\n\t"
                             "ldp x7, x8, [%[in], #96]\n\t"
                             "ldp x9, x10, [%[in], #112]\n\t"
                             "stp x3, x4, [%[out], #64]\n\t"
                             "stp x5, x6, [%[out], #80]\n\t"
                             "stp x7, x8, [%[out], #96]\n\t"
                             "stp x9, x10, [%[out], #112]\n\t"
                             "add %[in], %[in], #128\n\t"
                             "add %[out], %[out], #128\n\t"
                             "subs %[blocks], %[blocks], #1\n\t"
                             "b.ne 1b"
                             : [out] "+&r"(out), [in] "+&r"(in), [blocks] "+&r"(blocks)
                             :
                             : "x3", "x4", "x5", "x6", "x7", "x8", "x9", "x10", "cc", "memory");
#endif
    }
    count &= 127;
    if (backwards)
    {
        while (count >= 8)
        {
            out -= 8;
            in -= 8;
            uint64_t value = arm64_load_word(in);
            arm64_store_word(out, value);
            count -= 8;
        }
        while (count--)
            *(volatile unsigned char *)--out = *(const volatile unsigned char *)--in;
    }
    else
    {
        while (count >= 8)
        {
            uint64_t value = arm64_load_word(in);
            arm64_store_word(out, value);
            out += 8;
            in += 8;
            count -= 8;
        }
        while (count--)
            *(volatile unsigned char *)out++ = *(const volatile unsigned char *)in++;
    }
    return destination;
}

static __attribute__((always_inline)) inline void *arm64_set(void *destination, unsigned char value,
                                                             size_t count)
{
    unsigned char *out = destination;
    uint64_t repeated = (uint64_t)value * UINT64_C(0x0101010101010101);
    size_t blocks = count / 128;
    if (blocks)
    {
#if defined(MEMORY_TEST_ARM64)
        typedef uint64_t unaligned_word __attribute__((__may_alias__, __aligned__(1)));
        do
        {
            for (unsigned i = 0; i < 16; ++i)
                ((volatile unaligned_word *)out)[i] = repeated;
            out += 128;
        } while (--blocks);
#else
        __asm__ volatile("1:\n\t"
                         "stp %[fill], %[fill], [%[out], #112]\n\t"
                         "stp %[fill], %[fill], [%[out], #96]\n\t"
                         "stp %[fill], %[fill], [%[out], #80]\n\t"
                         "stp %[fill], %[fill], [%[out], #64]\n\t"
                         "stp %[fill], %[fill], [%[out], #48]\n\t"
                         "stp %[fill], %[fill], [%[out], #32]\n\t"
                         "stp %[fill], %[fill], [%[out], #16]\n\t"
                         "stp %[fill], %[fill], [%[out]], #128\n\t"
                         "subs %[blocks], %[blocks], #1\n\t"
                         "b.ne 1b"
                         : [out] "+&r"(out), [blocks] "+&r"(blocks)
                         : [fill] "r"(repeated)
                         : "cc", "memory");
#endif
    }
    count &= 127;
    while (count >= 8)
    {
        arm64_store_word(out, repeated);
        out += 8;
        count -= 8;
    }
    while (count--)
        *(volatile unsigned char *)out++ = value;
    return destination;
}
#endif

MEMORY_PRIMITIVE void *memset(void *destination, int value, size_t count)
{
#if defined(__aarch64__) && defined(LUME_AARCH64_STRING_ASM)
    return __memset_aarch64(destination, value, count);
#elif defined(__aarch64__)
    return arm64_set(destination, (unsigned char)value, count);
#else
    volatile unsigned char *p = destination;
    unsigned char byte = (unsigned char)value;
#if defined(__i386__)
    while (count && ((uintptr_t)p & 3))
    {
        *p++ = byte;
        --count;
    }
    uint32_t repeated = (uint32_t)byte * UINT32_C(0x01010101);
    size_t words = count / 4;
    __asm__ volatile("cld\n\trep stosl" : "+D"(p), "+c"(words) : "a"(repeated) : "cc", "memory");
    count &= 3;
#else
    memory_word repeated = (memory_word)byte * (UINTPTR_MAX / UCHAR_MAX);

    while (count && ((uintptr_t)p & (sizeof(memory_word) - 1)))
    {
        *p++ = byte;
        --count;
    }
    while (count >= sizeof(memory_word))
    {
        *(volatile memory_word *)p = repeated;
        p += sizeof(memory_word);
        count -= sizeof(memory_word);
    }
#endif
    while (count--)
        *p++ = byte;
    return destination;
#endif
}

MEMORY_PRIMITIVE void *memcpy(void *destination, const void *source, size_t count)
{
#if defined(__aarch64__) && defined(LUME_AARCH64_STRING_ASM)
    return __memcpy_aarch64_simd(destination, source, count);
#elif defined(__aarch64__)
    return arm64_copy(destination, source, count, 0);
#else
    volatile unsigned char *out = destination;
    const volatile unsigned char *in = source;
#if defined(__i386__)
    while (count && ((uintptr_t)out & 3))
    {
        *out++ = *in++;
        --count;
    }
    size_t words = count / 4;
    __asm__ volatile("cld\n\trep movsl" : "+D"(out), "+S"(in), "+c"(words) : : "cc", "memory");
    count &= 3;
#else

    if (!(((uintptr_t)out ^ (uintptr_t)in) & (sizeof(memory_word) - 1)))
    {
        while (count && ((uintptr_t)out & (sizeof(memory_word) - 1)))
        {
            *out++ = *in++;
            --count;
        }
        while (count >= sizeof(memory_word))
        {
            *(volatile memory_word *)out = *(const volatile memory_word *)in;
            out += sizeof(memory_word);
            in += sizeof(memory_word);
            count -= sizeof(memory_word);
        }
    }
#endif
    while (count--)
        *out++ = *in++;
    return destination;
#endif
}

MEMORY_PRIMITIVE void *memmove(void *to, const void *from, size_t n)
{
#if defined(__aarch64__) && defined(LUME_AARCH64_STRING_ASM)
    return __memmove_aarch64_simd(to, from, n);
#elif defined(__aarch64__)
    if (!n || to == from)
        return to;
    return arm64_copy(to, from, n,
                      (uintptr_t)to > (uintptr_t)from && (uintptr_t)to - (uintptr_t)from < n);
#else
    if (!n || to == from)
        return to;
    uintptr_t destination = (uintptr_t)to, source = (uintptr_t)from;
    if (destination < source ? source - destination >= n : destination - source >= n)
        return memcpy(to, from, n);
    volatile unsigned char *d = to;
    const volatile unsigned char *s = from;
    if ((uintptr_t)d < (uintptr_t)s)
        for (size_t i = 0; i < n; ++i)
            d[i] = s[i];
    else
        while (n)
        {
            --n;
            d[n] = s[n];
        }
    return to;
#endif
}
