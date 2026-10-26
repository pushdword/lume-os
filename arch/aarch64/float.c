#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

_Static_assert(LDBL_MANT_DIG == 113 && sizeof(long double) == 16, "AArch64 binary128 ABI");

int __fpclassifyl(long double value)
{
    union
    {
        long double value;
        uint64_t words[2];
    } bits = {.value = value};

    uint64_t high = bits.words[1] & UINT64_C(0x7fffffffffffffff);
    unsigned exponent = (unsigned)(high >> 48);
    bool fraction = (high & UINT64_C(0xffffffffffff)) || bits.words[0];
    if (!exponent)
        return fraction ? FP_SUBNORMAL : FP_ZERO;
    if (exponent == 0x7fff)
        return fraction ? FP_NAN : FP_INFINITE;
    return FP_NORMAL;
}
