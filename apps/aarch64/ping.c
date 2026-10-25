#include "network_tool.h"
#include <fd.h>

static bool input_enabled;
static struct lume_pollfd input_poll = {.fd = 0, .events = LUME_POLLIN};

static bool cancelled(void)
{
    if (!input_enabled)
        return false;
    for (unsigned i = 0; i < 32; ++i)
    {
        struct lume_fd_request poll = {.version = LUME_FD_VERSION,
                                       .bytes = sizeof(poll),
                                       .fd = -1,
                                       .buffer = (uint32_t)(uintptr_t)&input_poll,
                                       .count = 1};
        if (call(LUME_SYS_FD, LUME_FD_POLL, (uintptr_t)&poll, sizeof(poll)) <= 0)
            break;
        unsigned char byte;
        if (read(0, &byte, 1) != 1)
            break;
        if (byte == 3)
            return true;
    }
    return false;
}

static bool pause_ping(unsigned ms)
{
    while (ms)
    {
        if (cancelled())
            return false;
        unsigned delay = ms > 10 ? 10 : ms;
        tool_sleep(delay);
        ms -= delay;
    }
    return !cancelled();
}

int main(int argc, char **argv)
{
    unsigned count = 4, interval = 1;
    const char *host = 0;
    for (int i = 1; i < argc; ++i)
    {
        if (eq(argv[i], "-n"))
            continue;
        if (eq(argv[i], "-c") || eq(argv[i], "-i"))
        {
            bool number = eq(argv[i], "-c");
            unsigned value;
            if (++i == argc || !tool_unsigned(argv[i], number ? 10000 : 3600, &value) ||
                (number && !value))
                goto usage;
            if (number)
                count = value;
            else
                interval = value;
        }
        else if (eq(argv[i], "--help"))
        {
            tool_text(1, "usage: ping [-n] [-c COUNT] [-i SECONDS] HOST\n");
            return 0;
        }
        else if (argv[i][0] == '-' || host)
            goto usage;
        else
            host = argv[i];
    }
    if (!host)
        goto usage;
    unsigned char address[4];
    if (!tool_resolve(host, address, false))
    {
        tool_text(2, "ping: cannot resolve host\n");
        return 1;
    }
    tool_text(1, "PING ");
    tool_text(1, host);
    tool_text(1, " (");
    tool_ip(1, address);
    tool_text(1, ") 24 ICMP bytes\n");
    struct lume_stat input;
    input_enabled = !call(LUME_SYS_STAT, 0, (uintptr_t)&input, 0) &&
                    (input.kind == LUME_NODE_CHAR || input.kind == LUME_NODE_PIPE);
    unsigned received = 0, transmitted = 0;
    bool interrupted = false;
    uint64_t total = 0, minimum = UINT64_MAX, maximum = 0;
    for (unsigned i = 1; i <= count; ++i)
    {
        if (cancelled())
        {
            interrupted = true;
            break;
        }
        struct lume_stream_request request = {.version = LUME_IO_VERSION};
        for (unsigned j = 0; j < 4; ++j)
            request.address[j] = address[j];
        uint64_t start = tool_now();
        bool ok = false;
        int64_t result =
            call(LUME_SYS_STREAM, LUME_ICMP_ECHO_START, (uintptr_t)&request, sizeof(request));
        ++transmitted;
        if (!result)
        {
            for (;;)
            {
                if (cancelled())
                {
                    interrupted = true;
                    break;
                }
                result = call(LUME_SYS_STREAM, LUME_ICMP_ECHO_POLL, (uintptr_t)&request,
                              sizeof(request));
                if (result < 0 || (result & (LUME_STREAM_READABLE | LUME_STREAM_ERROR)))
                {
                    ok = result >= 0 && (result & LUME_STREAM_READABLE);
                    break;
                }
                tool_sleep(10);
            }
            (void)call(LUME_SYS_STREAM, LUME_ICMP_ECHO_CLOSE, (uintptr_t)&request, sizeof(request));
        }
        if (interrupted)
            break;
        uint64_t elapsed = tool_now() - start;
        if (ok)
        {
            ++received;
            total += elapsed;
            if (elapsed < minimum)
                minimum = elapsed;
            if (elapsed > maximum)
                maximum = elapsed;
            tool_text(1, "24 bytes from ");
            tool_ip(1, address);
            tool_text(1, ": icmp_seq=");
            tool_number(1, i);
            tool_text(1, " time=");
            tool_number(1, elapsed);
            tool_text(1, " ms\n");
        }
        else
        {
            tool_text(1, "Request timeout for icmp_seq=");
            tool_number(1, i);
            tool_text(1, "\n");
        }
        uint64_t delay = (uint64_t)interval * 1000;
        if (i != count && elapsed < delay && !pause_ping((unsigned)(delay - elapsed)))
        {
            interrupted = true;
            break;
        }
    }
    if (interrupted)
        tool_text(1, "^C\n");
    tool_text(1, "--- ");
    tool_text(1, host);
    tool_text(1, " ping statistics ---\n");
    tool_number(1, transmitted);
    tool_text(1, " packets transmitted, ");
    tool_number(1, received);
    tool_text(1, " received, ");
    tool_number(1, transmitted ? (transmitted - received) * 100 / transmitted : 0);
    tool_text(1, "% packet loss\n");
    if (received)
    {
        tool_text(1, "rtt min/avg/max = ");
        tool_number(1, minimum);
        tool_text(1, "/");
        tool_number(1, total / received);
        tool_text(1, "/");
        tool_number(1, maximum);
        tool_text(1, " ms\n");
    }
    return interrupted ? 130 : received ? 0 : 1;
usage:
    tool_text(2, "usage: ping [-n] [-c COUNT] [-i SECONDS] HOST\n");
    return 2;
}
