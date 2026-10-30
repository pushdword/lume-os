#include "../../libc/posix/terminal_read.h"
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

static struct
{
    uint64_t now, at[4];
    const char *data[4];
    unsigned next, available, consumed, waits;
    int error, wait_error;
    unsigned retries;
    bool hangup;
} input;

static uint64_t clock_value(void *context)
{
    (void)context;
    return input.now;
}

static int64_t read_value(void *context, void *buffer, size_t bytes)
{
    (void)context;
    if (input.error)
        return input.error;
    if (!input.available && input.data[input.next] && input.at[input.next] <= input.now)
        input.available = (unsigned)strlen(input.data[input.next]);
    if (!input.available)
        return input.hangup ? 0 : -11;
    if (bytes > input.available)
        bytes = input.available;
    memcpy(buffer, input.data[input.next] + input.consumed, bytes);
    input.consumed += (unsigned)bytes;
    input.available -= (unsigned)bytes;
    if (!input.available)
    {
        ++input.next;
        input.consumed = 0;
    }
    return (int64_t)bytes;
}

static int wait_value(void *context, uint64_t timeout)
{
    (void)context;
    ++input.waits;
    if (input.retries)
    {
        --input.retries;
        return -11;
    }
    if (input.wait_error)
        return input.wait_error;
    uint64_t next = input.data[input.next] ? input.at[input.next] : UINT64_MAX;
    uint64_t end = timeout == UINT64_MAX ? UINT64_MAX : input.now + timeout;
    assert(next != UINT64_MAX || end != UINT64_MAX);
    input.now = next < end ? next : end;
    return next <= end && next != UINT64_MAX;
}

int main(void)
{
    struct lume_terminal_reader reader = {clock_value, read_value, wait_value, NULL};
    char buffer[32];
    assert(lume_terminal_read(&reader, buffer, sizeof(buffer), 0, 0) == 0 && !input.waits);
    input.error = -14;
    assert(lume_terminal_read(&reader, buffer, sizeof(buffer), 0, 0) == -14);
    memset(&input, 0, sizeof(input));
    input.data[0] = "abcdef";
    assert(lume_terminal_read(&reader, buffer, 4, 0, 0) == 4 && !memcmp(buffer, "abcd", 4));
    memset(&input, 0, sizeof(input));
    input.data[0] = "a";
    input.data[1] = "bcd";
    input.at[0] = 30;
    input.at[1] = 60;
    assert(lume_terminal_read(&reader, buffer, sizeof(buffer), 4, 0) == 4);
    assert(input.now == 60 && !memcmp(buffer, "abcd", 4));
    memset(&input, 0, sizeof(input));
    input.data[0] = "abcd";
    input.at[0] = 20;
    assert(lume_terminal_read(&reader, buffer, 2, 4, 0) == 2 && input.now == 20);
    memset(&input, 0, sizeof(input));
    assert(lume_terminal_read(&reader, buffer, sizeof(buffer), 0, 1) == 0 && input.now == 100);
    memset(&input, 0, sizeof(input));
    input.data[0] = "ab";
    input.at[0] = 40;
    assert(lume_terminal_read(&reader, buffer, sizeof(buffer), 0, 1) == 2 && input.now == 40);
    memset(&input, 0, sizeof(input));
    input.data[0] = "a";
    input.data[1] = "b";
    input.at[0] = 500;
    input.at[1] = 550;
    assert(lume_terminal_read(&reader, buffer, sizeof(buffer), 4, 1) == 2 && input.now == 650);
    memset(&input, 0, sizeof(input));
    input.data[0] = "abc";
    assert(lume_terminal_read(&reader, buffer, sizeof(buffer), 4, 2) == 3 && input.now == 200);
    memset(&input, 0, sizeof(input));
    input.wait_error = -4;
    assert(lume_terminal_read(&reader, buffer, sizeof(buffer), 4, 0) == -4);
    input.data[0] = "ab";
    assert(lume_terminal_read(&reader, buffer, sizeof(buffer), 4, 0) == 2);
    memset(&input, 0, sizeof(input));
    input.retries = 1;
    input.data[0] = "retry";
    input.at[0] = 20;
    assert(lume_terminal_read(&reader, buffer, sizeof(buffer), 1, 0) == 5);
    assert(input.waits == 2 && !memcmp(buffer, "retry", 5));
    memset(&input, 0, sizeof(input));
    input.hangup = true;
    assert(lume_terminal_read(&reader, buffer, sizeof(buffer), 4, 2) == 0);
    puts(
        "PASS terminal reads with minimum bytes, initial timeout, inter-byte timeout and interruption");
}
