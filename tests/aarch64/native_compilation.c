#include "native_compilation.h"
#include <stdio.h>
#include <stdlib.h>

_Static_assert(sizeof(void *) == 8 && sizeof(long) == 8, "AArch64 LP64 types");

int main(void)
{
    struct native_pair sum = native_add_pair((struct native_pair){1.25, 2.5},
                                             (struct native_pair){2.25, 3.75});
    if (sum.first != 3.5 || sum.second != 6.25)
        return 1;
    struct native_words words = native_make_words(UINT64_C(0x123456789abcdef0));
    for (unsigned i = 0; i < 4; ++i)
        if (words.values[i] != UINT64_C(0x123456789abcdef0) + i)
            return 2;
    if (native_sum(3, 1.5, 2.0, 3.25) != 6.75)
        return 3;
    char *end;
    long double value = strtold("1.25", &end);
    if (*end || value != 1.25L)
        return 4;
    puts("NATIVE_C_COMPILATION_PASS HFA/sret/varargs/binary128");
    return 0;
}
