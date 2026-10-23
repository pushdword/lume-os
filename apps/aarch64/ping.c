#include "network_tool.h"

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
    struct lume_net_request request = {0};
    unsigned position = 0;
    for (unsigned i = 0; i < 4; ++i)
    {
        if (i)
            request.argument[position++] = '.';
        unsigned value = address[i];
        if (value >= 100)
            request.argument[position++] = (char)('0' + value / 100);
        if (value >= 10)
            request.argument[position++] = (char)('0' + (value / 10) % 10);
        request.argument[position++] = (char)('0' + value % 10);
    }
    unsigned received = 0;
    uint64_t total = 0, minimum = UINT64_MAX, maximum = 0;
    for (unsigned i = 1; i <= count; ++i)
    {
        uint64_t start = tool_now();
        bool ok = call(LUME_SYS_NET, LUME_NET_PING, (uintptr_t)&request, sizeof(request)) >= 0;
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
        if (i != count && elapsed < delay)
            tool_sleep((unsigned)(delay - elapsed));
    }
    tool_text(1, "--- ");
    tool_text(1, host);
    tool_text(1, " ping statistics ---\n");
    tool_number(1, count);
    tool_text(1, " packets transmitted, ");
    tool_number(1, received);
    tool_text(1, " received, ");
    tool_number(1, (count - received) * 100 / count);
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
    return received ? 0 : 1;
usage:
    tool_text(2, "usage: ping [-n] [-c COUNT] [-i SECONDS] HOST\n");
    return 2;
}
