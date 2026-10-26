#include "network_tool.h"

static struct lume_stream_request stream;

static int transfer(unsigned op)
{
    return (int)call(LUME_SYS_STREAM, op, (uintptr_t)&stream, sizeof(stream));
}

static bool output(int fd, const unsigned char *data, unsigned count)
{
    while (count)
    {
        int64_t n = write(fd, data, count);
        if (n <= 0 || (uint64_t)n > count)
            return false;
        data += n;
        count -= (unsigned)n;
    }
    return true;
}

int main(int argc, char **argv)
{
    bool udp = false, listen = false, scan = false, numeric = false, verbose = false;
    unsigned timeout = 10, port = 0, arguments = 0;
    int input_fd = 0, output_fd = 1;
    const char *input_path = 0, *output_path = 0;
    const char *positional[2];
    for (int i = 1; i < argc; ++i)
    {
        if (eq(argv[i], "--input") || eq(argv[i], "--output"))
        {
            bool input = eq(argv[i], "--input");
            if (++i == argc || (input ? input_path != 0 : output_path != 0))
                goto usage;
            if (input)
                input_path = argv[i];
            else
                output_path = argv[i];
            continue;
        }
        if (eq(argv[i], "--help"))
        {
            tool_text(
                1,
                "usage: nc [-nuvzN] [-w SECONDS] HOST PORT\n       nc -l [-nuvN] [-w SECONDS] [-p PORT] [ADDRESS] [PORT]\n       [--input FILE] [--output FILE]\n");
            return 0;
        }
        if (argv[i][0] == '-' && argv[i][1])
        {
            const char *flag = argv[i] + 1;
            while (*flag)
            {
                char option = *flag++;
                if (option == 'u')
                    udp = true;
                else if (option == 'l')
                    listen = true;
                else if (option == 'z')
                    scan = true;
                else if (option == 'n')
                    numeric = true;
                else if (option == 'v')
                    verbose = true;
                else if (option == 'N')
                    continue;
                else if (option == 'w' || option == 'p')
                {
                    const char *value = *flag ? flag : (++i < argc ? argv[i] : "");
                    unsigned n;
                    if (!tool_unsigned(value, option == 'w' ? 3600 : 65535, &n) || !n)
                        goto usage;
                    if (option == 'w')
                        timeout = n;
                    else
                        port = n;
                    break;
                }
                else
                    goto usage;
            }
        }
        else if (arguments < 2)
            positional[arguments++] = argv[i];
        else
            goto usage;
    }
    if (scan && (udp || listen || input_path || output_path))
        goto usage;
    const char *host = "0.0.0.0";
    if (!listen)
    {
        if (arguments != 2 || port || !tool_unsigned(positional[1], 65535, &port) || !port)
            goto usage;
        host = positional[0];
    }
    else if (port)
    {
        if (arguments > 1)
            goto usage;
        if (arguments)
            host = positional[0];
    }
    else
    {
        if (!arguments || !tool_unsigned(positional[arguments - 1], 65535, &port) || !port)
            goto usage;
        if (arguments == 2)
            host = positional[0];
    }
    stream.version = LUME_IO_VERSION;
    stream.port = port;
    if (!tool_resolve(host, stream.address, numeric))
    {
        tool_text(2, "nc: cannot resolve IPv4 address\n");
        return 1;
    }
    int result = 1;
    unsigned op = udp ? (listen ? LUME_DATAGRAM_LISTEN : LUME_DATAGRAM_CONNECT)
                      : (listen ? LUME_STREAM_LISTEN : LUME_STREAM_CONNECT_ASYNC);
    if (transfer(op) < 0)
    {
        tool_text(2, "nc: cannot open endpoint\n");
        return 1;
    }
    uint64_t start = tool_now(), limit = (uint64_t)timeout * 1000;
    if (listen)
    {
        tool_text(2, "nc: listening on ");
        tool_number(2, port);
        tool_text(2, udp ? " UDP\n" : " TCP\n");
    }
    if (!udp)
    {
        for (;;)
        {
            int events = transfer(LUME_STREAM_POLL);
            if (events < 0 || (events & LUME_STREAM_ERROR) || tool_now() - start >= limit)
            {
                tool_text(2, "nc: connection failed or timed out\n");
                goto done;
            }
            if (listen && (events & LUME_STREAM_READABLE))
            {
                unsigned listener = stream.handle;
                if (transfer(LUME_STREAM_ACCEPT) < 0)
                    goto done;
                unsigned child = stream.handle;
                stream.handle = listener;
                (void)transfer(LUME_STREAM_CLOSE);
                stream.handle = child;
                break;
            }
            if (!listen && (events & LUME_STREAM_WRITABLE))
                break;
            tool_sleep(1);
        }
    }
    if (verbose)
        tool_text(2, udp ? "nc: UDP endpoint ready\n" : "nc: TCP connected\n");
    if (scan)
    {
        result = 0;
        goto done;
    }
    if (input_path && (input_fd = (int)open(input_path, LUME_O_RDONLY)) < 0)
    {
        tool_text(2, "nc: cannot open input\n");
        goto done;
    }
    if (output_path &&
        (output_fd = (int)open(output_path, LUME_O_WRONLY | LUME_O_CREAT | LUME_O_TRUNC)) < 0)
    {
        tool_text(2, "nc: cannot open output\n");
        goto done;
    }
    struct lume_stat info;
    if (call(LUME_SYS_STAT, (unsigned)input_fd, (uintptr_t)&info, 0) < 0)
        goto done;
    bool tty = info.kind == LUME_NODE_CHAR;
    bool input_done = false, output_done = false, shutdown = false;
    unsigned char pending[LUME_IO_BYTES];
    unsigned first = 0, count = 0;
    uint64_t sent = 0, received = 0;
    start = tool_now();
    for (;;)
    {
        int events = transfer(LUME_STREAM_POLL);
        if (events < 0)
            goto done;
        bool active = false;
        if (!output_done && (events & LUME_STREAM_READABLE))
        {
            stream.count = LUME_IO_BYTES;
            int n = transfer(LUME_STREAM_RECEIVE);
            if (n < 0 && n != -LUME_EAGAIN)
                goto done;
            if (n >= 0)
            {
                if (!udp && !n)
                    output_done = true;
                else
                {
                    if (!output(output_fd, stream.data, (unsigned)n))
                        goto done;
                    received += (unsigned)n;
                    active = true;
                }
            }
        }
        if (events & LUME_STREAM_ERROR)
        {
            tool_text(2, "nc: connection reset\n");
            goto done;
        }
        if (!input_done && !count && (events & LUME_STREAM_WRITABLE) &&
            (!tty || call(LUME_SYS_TERMINAL, 1, 0, 0) > 0))
        {
            int64_t n = read(input_fd, pending, udp ? 1200 : sizeof(pending));
            if (tty && n == 1 && (pending[0] == 3 || pending[0] == 4))
            {
                if (pending[0] == 3)
                {
                    result = 130;
                    goto done;
                }
                n = 0;
            }
            if (n < 0 && n != -LUME_EAGAIN)
                goto done;
            if (!n)
                input_done = true;
            if (n > 0)
            {
                first = 0;
                count = (unsigned)n;
            }
        }
        if (count && (events & LUME_STREAM_WRITABLE))
        {
            stream.count = count;
            for (unsigned i = 0; i < count; ++i)
                stream.data[i] = pending[first + i];
            int n = transfer(LUME_STREAM_SEND);
            if (n < 0 && n != -LUME_EAGAIN)
                goto done;
            if (n > 0)
            {
                first += (unsigned)n;
                count -= (unsigned)n;
                sent += (unsigned)n;
                active = true;
            }
        }
        if (!udp && input_done && !count && !shutdown)
        {
            int status = transfer(LUME_STREAM_SHUTDOWN_WRITE);
            if (status < 0 && status != -LUME_EAGAIN)
                goto done;
            shutdown = !status;
        }
        if (!udp && output_done && shutdown)
        {
            result = 0;
            goto done;
        }
        if (active)
            start = tool_now();
        if (tool_now() - start >= limit)
        {
            if (udp && (sent || received))
                result = 0;
            else
                tool_text(2, "nc: idle timeout\n");
            goto done;
        }
        tool_sleep(1);
    }
done:
    (void)transfer(LUME_STREAM_CLOSE);
    if (input_fd > 2)
        (void)close(input_fd);
    if (output_fd > 2)
        (void)close(output_fd);
    return result;
usage:
    tool_text(
        2,
        "usage: nc [-nuvzN] [-w SECONDS] HOST PORT\n       nc -l [-nuvN] [-w SECONDS] [-p PORT] [ADDRESS] [PORT]\n       [--input FILE] [--output FILE]\n");
    return 2;
}
