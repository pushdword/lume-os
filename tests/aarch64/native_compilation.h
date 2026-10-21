#ifndef LUME_NATIVE_COMPILATION_H
#define LUME_NATIVE_COMPILATION_H
#include <stdint.h>
struct native_pair
{
    double first, second;
};
struct native_words
{
    uint64_t values[4];
};
struct native_pair native_add_pair(struct native_pair, struct native_pair);
struct native_words native_make_words(uint64_t);
double native_sum(unsigned, ...);
#endif
