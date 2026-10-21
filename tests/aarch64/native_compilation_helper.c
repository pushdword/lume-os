#include "native_compilation.h"
#include <stdarg.h>

struct native_pair native_add_pair(struct native_pair a, struct native_pair b)
{
    return (struct native_pair){a.first + b.first, a.second + b.second};
}

struct native_words native_make_words(uint64_t base)
{
    return (struct native_words){{base, base + 1, base + 2, base + 3}};
}

double native_sum(unsigned count, ...)
{
    va_list arguments;
    va_start(arguments, count);
    double sum = 0;
    while (count--)
        sum += va_arg(arguments, double);
    va_end(arguments);
    return sum;
}
