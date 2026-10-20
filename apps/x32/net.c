#include "net_client.h"

int app_main(int argc, char **argv)
{
    if (argc == 1 || app_equal(argv[1], "info"))
    {
        struct lume_net_info info;
        int r = net_info(&info);
        if (r < 0)
        {
            app_error("net", r);
            return 1;
        }
        app_puts("W5500 ");
        app_puts(info.flags & LUME_NET_LINK ? "link up" : "link down");
        app_puts(" IP ");
        net_ip(info.configuration.address);
        app_puts(" mask ");
        net_ip(info.configuration.netmask);
        app_puts(" gateway ");
        net_ip(info.configuration.gateway);
        app_puts("\nsockets ");
        app_number(info.sockets_used);
        app_puts("/");
        app_number(info.socket_count);
        app_puts(" SPI ");
        app_number(info.spi_hz);
        app_puts(" Hz\n");
        return 0;
    }
    if (argc == 5 && app_equal(argv[1], "nc"))
    {
        unsigned char ip[4];
        unsigned port = 0;
        const char *p = argv[3];
        while (*p >= '0' && *p <= '9')
        {
            port = port * 10u + (unsigned)(*p++ - '0');
            if (port > 65535)
                return 2;
        }
        if (*p || !port || !net_parse_ip(argv[2], ip))
            return 2;
        int fd = net_socket(LUME_NET_TCP);
        if (fd < 0)
        {
            app_error("socket", fd);
            return 1;
        }
        struct lume_net_request q = net_request(fd);
        memcpy(q.address, ip, 4);
        q.port = port;
        int r = net_call(LUME_NET_CONNECT, &q);
        uint32_t start = net_now();
        while (r == -LUME_NET_EINPROGRESS || r == -LUME_NET_EAGAIN)
        {
            q.events = LUME_NET_POLLOUT;
            r = net_call(LUME_NET_POLL, &q);
            if (r > 0 && (q.events & LUME_NET_POLLOUT))
                break;
            if (r < 0 || (q.events & (LUME_NET_POLLERR | LUME_NET_POLLHUP)) ||
                net_now() - start > 10000u)
            {
                r = -LUME_NET_ETIMEDOUT;
                break;
            }
            net_idle(0);
            r = -LUME_NET_EINPROGRESS;
        }
        if (r < 0)
        {
            app_error("connect", r);
            net_close(fd);
            return 1;
        }
        unsigned n = app_length(argv[4]), offset = 0;
        char buffer[1024];
        start = net_now();
        while (offset < n)
        {
            unsigned chunk = n - offset;
            if (chunk > 1024)
                chunk = 1024;
            r = net_io(LUME_NET_SEND, fd, argv[4] + offset, chunk);
            if (r > 0)
            {
                offset += (unsigned)r;
                start = net_now();
            }
            else if (r != -LUME_NET_EAGAIN || net_now() - start > 10000u)
                break;
            else
                net_idle(0);
        }
        start = net_now();
        while (offset == n && net_now() - start < 5000u)
        {
            r = net_io(LUME_NET_RECV, fd, buffer, sizeof buffer);
            if (r > 0)
            {
                lume_syscall(LUME_WRITE, (uint32_t)(uintptr_t)buffer, (uint32_t)r, 0);
                start = net_now();
            }
            else if (r == 0 || r != -LUME_NET_EAGAIN)
                break;
            else
                net_idle(0);
        }
        net_close(fd);
        return offset == n ? 0 : 1;
    }
    app_puts("net info | net nc ADDRESS PORT TEXT\n");
    return 2;
}
