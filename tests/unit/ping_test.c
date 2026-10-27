#define LUME_USER_HOST_TEST 1
#define main ping_main
#define puts user_puts
#include "ping.c"
#undef main
#undef puts
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint64_t milliseconds, cancel_at = UINT64_MAX;
static unsigned starts, closes;
static bool pending, active;
static char output[4096];
static size_t output_used;
static unsigned write_calls;

int64_t call(uint64_t number, uint64_t a, uint64_t b, uint64_t c)
{
    if (number == LUME_SYS_CLOCK)
        return (int64_t)milliseconds;
    if (number == LUME_SYS_SLEEP)
    {
        milliseconds += a;
        return 0;
    }
    if (number == LUME_SYS_STAT)
    {
        ((struct lume_stat *)(uintptr_t)b)->kind = LUME_NODE_CHAR;
        return 0;
    }
    if (number == LUME_SYS_FD)
    {
        assert(a == LUME_FD_POLL);
        return milliseconds >= cancel_at ? 1 : 0;
    }
    if (number == LUME_SYS_READ)
    {
        assert(a == 0 && c == 1 && milliseconds >= cancel_at);
        *(unsigned char *)(uintptr_t)b = 3;
        cancel_at = UINT64_MAX;
        return 1;
    }
    if (number == LUME_SYS_WRITE)
    {
        if (++write_calls % 7 == 1)
            return -4;
        if (c > 3)
            c = 3;
        assert(output_used + c < sizeof(output));
        memcpy(output + output_used, (void *)(uintptr_t)b, c);
        output_used += c;
        output[output_used] = 0;
        return (int64_t)c;
    }
    if (number == LUME_SYS_STREAM)
    {
        struct lume_stream_request *r = (void *)(uintptr_t)b;
        if (a == LUME_ICMP_ECHO_START)
        {
            assert(!active && r->address[0] == 10);
            active = true;
            r->handle = ++starts;
            return 0;
        }
        assert(active && r->handle == starts);
        if (a == LUME_ICMP_ECHO_POLL)
            return pending ? 0 : LUME_STREAM_READABLE;
        assert(a == LUME_ICMP_ECHO_CLOSE);
        active = false;
        ++closes;
        return 0;
    }
    assert(0);
    return -LUME_ENOSYS;
}

static void reset(void)
{
    milliseconds = starts = closes = output_used = 0;
    output[0] = 0;
    active = false;
    write_calls = 0;
    cancel_at = UINT64_MAX;
}

int main(void)
{
    char *args[] = {"ping", "-c", "100", "10.0.3.2"};
    reset();
    pending = true;
    cancel_at = 120;
    assert(ping_main(4, args) == 130);
    assert(milliseconds == 120 && starts == 1 && closes == 1 && !active);
    assert(strstr(output, "1 packets transmitted, 0 received") && strstr(output, "^C"));
    reset();
    pending = false;
    cancel_at = 120;
    assert(ping_main(4, args) == 130);
    assert(milliseconds == 120 && starts == 1 && closes == 1);
    assert(strstr(output, "1 packets transmitted, 1 received"));
    reset();
    args[2] = "4";
    assert(ping_main(4, args) == 0);
    assert(starts == 4 && closes == 4 && milliseconds == 3000);
    assert(strstr(output, "4 packets transmitted, 4 received"));
    puts("PASS_PING_CANCEL");
    return 0;
}
