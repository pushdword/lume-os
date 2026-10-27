#define app_main less_main
#include "../../apps/utils/less.c"
#undef app_main
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static char input[520], expected[sizeof(input)], output[sizeof(input)];
static unsigned input_at, output_bytes, write_calls, limit, stop_at, checks;
static int stop_result;
static bool interrupt;

#define T(expression)                                                                              \
    do                                                                                             \
    {                                                                                              \
        assert(expression);                                                                        \
        ++checks;                                                                                  \
    } while (0)

int32_t lume_syscall(uint32_t number, uint32_t a, uint32_t b, uint32_t c)
{
    if (number == LUME_READ)
    {
        T(a == 3 && b == (uint32_t)(uintptr_t)buffer && c == sizeof(buffer));
        unsigned count = sizeof(input) - input_at;
        if (count > c)
            count = c;
        memcpy(buffer, input + input_at, count);
        input_at += count;
        return (int32_t)count;
    }
    T(number == LUME_FDWRITE && a == 1);
    unsigned offset = b - (uint32_t)(uintptr_t)buffer;
    T(offset <= sizeof(buffer) && c <= sizeof(buffer) - offset);
    ++write_calls;
    if (interrupt)
    {
        interrupt = false;
        return -4;
    }
    if (stop_at && write_calls >= stop_at)
        return stop_result == INT32_MAX ? (int32_t)c + 1 : stop_result;
    if (c > limit)
        c = limit;
    T(c <= sizeof(output) - output_bytes);
    memcpy(output + output_bytes, buffer + offset, c);
    output_bytes += c;
    return (int32_t)c;
}

static void reset(unsigned cap)
{
    input_at = output_bytes = write_calls = stop_at = 0;
    stop_result = 0;
    limit = cap;
    interrupt = false;
    memset(output, 0, sizeof(output));
}

int main(void)
{
    for (unsigned i = 0; i < sizeof(input); ++i)
        input[i] = expected[i] = (char)('a' + i % 26);
    input[10] = 1;
    input[300] = (char)0xff;
    expected[10] = expected[300] = '.';
    for (unsigned cap = 1; cap <= 64; cap *= 64)
    {
        reset(cap);
        interrupt = true;
        uint32_t position = 0;
        int end;
        T(page(3, 20, &position, &end) == 0);
        T(end && position == sizeof(input) && output_bytes == sizeof(input));
        T(!memcmp(output, expected, sizeof(output)));
    }
    const int stops[] = {-11, 0, INT32_MAX};
    for (unsigned i = 0; i < sizeof(stops) / sizeof(stops[0]); ++i)
    {
        reset(64);
        stop_at = 2;
        stop_result = stops[i];
        uint32_t position = 0;
        int end;
        T(page(3, 20, &position, &end) == -5);
        T(write_calls == 2 && output_bytes == 64);
        T(!memcmp(output, expected, output_bytes));
    }
    printf("PASS_LESS_OUTPUT checks=%u short/EINTR/blocked/zero/bounds/sanitized\n", checks);
    return 0;
}
