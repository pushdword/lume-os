#ifdef LUME_CONSOLE_API_TEST
#include "api.h"
#define output_helper app_puts
#else
#define LUME_USER_HOST_TEST 1
#define puts output_helper
#include "lume/app_support.h"
#undef puts
#endif
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static const char *input;
static size_t input_bytes, output_bytes;
static unsigned calls, checks, limit;
static unsigned target_fd = 1;
static bool variable;
static const int64_t *responses;
static unsigned response_count;
static char output[1024];

#define T(expression)                                                                              \
    do                                                                                             \
    {                                                                                              \
        assert(expression);                                                                        \
        ++checks;                                                                                  \
    } while (0)

static int64_t result(size_t bytes)
{
    T(++calls < 1024);
    T(bytes == input_bytes - output_bytes);
    int64_t count;
    if (calls <= response_count)
        count = responses[calls - 1];
    else
    {
        static const unsigned limits[] = {3, 1, 64, 7, 2};
        unsigned cap = variable ? limits[(calls - 1) % 5] : limit;
        count = bytes < cap ? (int64_t)bytes : cap;
    }
    if (count > 0 && (uint64_t)count <= bytes)
    {
        T(output_bytes + (size_t)count <= sizeof(output));
        memcpy(output + output_bytes, input + output_bytes, (size_t)count);
        output_bytes += (size_t)count;
    }
    return count;
}

#ifndef LUME_CONSOLE_API_TEST
int64_t call(uint64_t number, uint64_t fd, uint64_t address, uint64_t bytes)
{
    T(number == LUME_SYS_WRITE && fd == target_fd);
    T(address == (uintptr_t)(input + output_bytes));
    return result((size_t)bytes);
}
#else
int32_t lume_syscall(uint32_t number, uint32_t fd, uint32_t address, uint32_t bytes)
{
    T(number == LUME_FDWRITE && fd == target_fd);
    T(address == (uint32_t)(uintptr_t)(input + output_bytes));
    return (int32_t)result(bytes);
}
#endif

static void run(const char *text, unsigned cap, bool varying, const int64_t *script,
                unsigned script_count, size_t expected_bytes, unsigned expected_calls)
{
    input = text;
    input_bytes = strlen(text);
    output_bytes = calls = 0;
    memset(output, 0xa5, sizeof(output));
    limit = cap;
    variable = varying;
    responses = script;
    response_count = script_count;
    output_helper(text);
    T(output_bytes == expected_bytes);
    T(!memcmp(output, text, expected_bytes));
    T((unsigned char)output[expected_bytes] == 0xa5);
    T(expected_calls ? calls == expected_calls : calls > 1);
}

static void run_all(const char *text, size_t bytes, unsigned fd, const int64_t *script,
                    unsigned script_count, int64_t expected, unsigned expected_calls)
{
    input = text;
    input_bytes = bytes;
    target_fd = fd;
    output_bytes = calls = 0;
    memset(output, 0xa5, sizeof(output));
    limit = 64;
    variable = false;
    responses = script;
    response_count = script_count;
#ifdef LUME_CONSOLE_API_TEST
    T(app_write_all((int)fd, text, (unsigned)bytes) == expected);
#else
    T(write_all((int)fd, text, bytes) == expected);
#endif
    size_t expected_bytes = expected > 0 ? (size_t)expected : 0;
    T(output_bytes == expected_bytes);
    if (expected_bytes)
        T(!memcmp(output, text, expected_bytes));
    T((unsigned char)output[expected_bytes] == 0xa5);
    T(calls == expected_calls);
}

int main(void)
{
#ifndef LUME_CONSOLE_API_TEST
    (void)eq;
    (void)open;
    (void)close;
    (void)read;
#endif
    char text[258];
    for (unsigned i = 0; i < sizeof(text) - 1; ++i)
        text[i] = (char)('a' + i % 26);
    text[2] = '\n';
    text[3] = (char)0xc3;
    text[4] = (char)0xa1;
    text[sizeof(text) - 1] = 0;
    const char terminated[] = {'a', 'b', 0, 'c', 'd', 0};
    static const int64_t interrupted[] = {-4, 1, -4, -4, 2};
    static const int64_t again[] = {2, -11, 1};
    static const int64_t error[] = {2, -5, 1};
    static const int64_t zero[] = {2, 0, 1};
    static const int64_t invalid[] = {2, INT32_MAX, 1};
    static const int64_t first_again[] = {-11, 1};
    static const int64_t first_zero[] = {0, 1};
    static const int64_t first_invalid[] = {INT32_MAX, 1};
    run(text, 1, false, NULL, 0, sizeof(text) - 1, sizeof(text) - 1);
    run(text, 64, false, NULL, 0, sizeof(text) - 1, 5);
    run(text, 64, true, NULL, 0, sizeof(text) - 1, 0);
    run(text, 64, false, interrupted, 5, sizeof(text) - 1, 9);
    run(text, 64, false, again, 3, 2, 2);
    run(text, 64, false, error, 3, 2, 2);
    run(text, 64, false, zero, 3, 2, 2);
    run(text, 64, false, invalid, 3, 2, 2);
    run(text, 64, false, first_again, 2, 0, 1);
    run(text, 64, false, first_zero, 2, 0, 1);
    run(text, 64, false, first_invalid, 2, 0, 1);
    run(terminated, 1, false, NULL, 0, 2, 2);
    input = "";
    input_bytes = output_bytes = calls = 0;
    output_helper(input);
    T(!calls && !output_bytes);
#ifndef LUME_CONSOLE_API_TEST
    static const int64_t huge[] = {INT64_MAX, 1};
    run(text, 64, false, huge, 2, 0, 1);
#endif
    static const int64_t first_error[] = {-5, 1};
    run_all(text, sizeof(text) - 1, 2, NULL, 0, sizeof(text) - 1, 5);
    run_all(terminated, sizeof(terminated), 7, interrupted, 5, sizeof(terminated), 6);
    run_all(text, sizeof(text) - 1, 7, again, 3, 2, 2);
    run_all(text, sizeof(text) - 1, 7, error, 3, 2, 2);
    run_all(text, sizeof(text) - 1, 7, zero, 3, 2, 2);
    run_all(text, sizeof(text) - 1, 7, invalid, 3, 2, 2);
    run_all(text, sizeof(text) - 1, 7, first_again, 2, -11, 1);
    run_all(text, sizeof(text) - 1, 7, first_error, 2, -5, 1);
    run_all(text, sizeof(text) - 1, 7, first_zero, 2, 0, 1);
    run_all(text, sizeof(text) - 1, 7, first_invalid, 2, -5, 1);
    run_all(NULL, 0, 7, NULL, 0, 0, 0);
#ifdef LUME_CONSOLE_API_TEST
    run_all(NULL, UINT32_MAX, 7, NULL, 0, -75, 0);
#else
    run_all(NULL, SIZE_MAX, 7, NULL, 0, -75, 0);
#endif
    printf("PASS_CONSOLE_OUTPUT checks=%u short/EINTR/errors/zero/bounds/NUL/order\n", checks);
    return 0;
}
