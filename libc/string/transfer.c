#include <stddef.h>
#include <stdint.h>
#include <limits.h>

typedef uintptr_t memory_word __attribute__((__may_alias__));

#if defined(__clang__)
#define MEMORY_PRIMITIVE __attribute__((no_builtin("memcpy", "memset", "memmove")))
#elif defined(__GNUC__)
#define MEMORY_PRIMITIVE __attribute__((optimize("no-tree-loop-distribute-patterns")))
#else
#define MEMORY_PRIMITIVE
#endif

#if defined(__aarch64__)
typedef uint64_t memory_vector __attribute__((__vector_size__(16), __may_alias__));
#endif

MEMORY_PRIMITIVE void *memset(void *destination, int value, size_t count)
{
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
#if defined(__aarch64__)
    if (count >= 64)
    {
        while ((uintptr_t)p & 15)
        {
            *p++ = byte;
            --count;
        }
        memory_vector block = {repeated, repeated};
        while (count >= 64)
        {
            volatile memory_vector *out = (volatile memory_vector *)p;
            out[0] = block;
            out[1] = block;
            out[2] = block;
            out[3] = block;
            p += 64;
            count -= 64;
        }
    }
#endif
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
}

MEMORY_PRIMITIVE void *memcpy(void *destination, const void *source, size_t count)
{
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
#if defined(__aarch64__)
    if (count >= 64 && !(((uintptr_t)out ^ (uintptr_t)in) & 15))
    {
        while ((uintptr_t)out & 15)
        {
            *out++ = *in++;
            --count;
        }
        while (count >= 64)
        {
            const memory_vector *from = (const memory_vector *)in;
            memory_vector a = from[0], b = from[1], c = from[2], d = from[3];
            memory_vector *to = (memory_vector *)out;
            to[0] = a;
            to[1] = b;
            to[2] = c;
            to[3] = d;
            out += 64;
            in += 64;
            count -= 64;
        }
    }
#endif
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
}

MEMORY_PRIMITIVE void *memmove(void *to, const void *from, size_t n)
{
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
}
