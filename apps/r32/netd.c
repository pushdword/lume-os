/* DHCP + bounded LNET1 file service. No remote command execution.
 * The service accepts clients on the configured LAN subnet.
 * Each upload is streamed, synced and read back before publication. FatFs has
 * no replace-rename: replacing an existing target includes an unlink window.
 */
#include "net_client.h"
#include "net_protocol.h"
#include "dhcp.h"
/* Socket syscalls remain bounded to1KiB. Aggregate filesystem work in4KiB
 * blocks and sync the exclusive staging file once before final readback. */
static unsigned char block[4096];
static unsigned sequence;
static struct lume_net_config configuration;
static uint32_t renewal;
static uint32_t expiry;

static void close_wait(int fd)
{
    uint32_t start = net_now();
    int result;
    do
    {
        result = net_close(fd);
        if (result == -LUME_NET_EAGAIN)
            net_idle(0);
    } while (result == -LUME_NET_EAGAIN && net_now() - start <= 5250u);
}

static int stream(int fd, void *data, unsigned count, int sending)
{
    unsigned char *p = data;
    uint32_t last = net_now();
    while (count)
    {
        unsigned n = count > LUME_NET_IO_MAX ? LUME_NET_IO_MAX : count;
        /* One clock sample controls this entire attempt, including successful
         * progress and EAGAIN. Unsigned deltas retain the30s wrap-safe limit. */
        uint32_t now = net_now();
        if (expiry && (int32_t)(now - expiry) >= 0)
            return 0;
        int r = net_io(sending ? LUME_NET_SEND : LUME_NET_RECV, fd, p, n);
        if (r > 0)
        {
            p += r;
            count -= (unsigned)r;
            last = now;
        }
        else if (r != -LUME_NET_EAGAIN || now - last > 30000u)
            return 0;
        else
            net_idle(0);
    }
    return 1;
}

static int answer(int fd, int status, uint32_t size, uint32_t crc)
{
    unsigned char b[24] = {'L', 'A', 'C', 'K', 1, 0, 0, 0};
    net_put32(b + 8, (uint32_t)status);
    net_put32(b + 12, size);
    net_put32(b + 16, crc);
    return stream(fd, b, sizeof b, 1);
}

static int unlink_path(const char *p)
{
    return lume_syscall(LUME_UNLINK, (uint32_t)(uintptr_t)p, 0, 0);
}

static int rename_path(const char *from, const char *to)
{
    return lume_syscall(LUME_RENAME, (uint32_t)(uintptr_t)from, (uint32_t)(uintptr_t)to, 0);
}

static int writable(const char *path)
{
    return !strncmp(path, "/root/", 6) || !strncmp(path, "/usr/bin/", 9) ||
           !strncmp(path, "/usr/lib/", 9) || !strncmp(path, "/usr/libexec/", 13) ||
           !strcmp(path, "/boot/lume.r32") || !strcmp(path, "/sbin/init" LUME_APP_SUFFIX "");
}

static int temporary(const char *target, char *out)
{
    unsigned prefix = 0;
    for (unsigned i = 0; target[i]; i++)
        if (target[i] == '/')
            prefix = i + 1;
    if (prefix + 12 > NET_PATH_MAX)
        return -LUME_NET_EINVAL;
    memcpy(out, target, prefix);
    const char hex[] = "0123456789ABCDEF";
    for (unsigned attempt = 0; attempt < 32; attempt++)
    {
        unsigned n = ++sequence;
        memcpy(out + prefix, "NWP00000.TMP", 13);
        for (unsigned i = 0; i < 5; i++)
        {
            out[prefix + 7 - i] = hex[n & 15u];
            n >>= 4;
        }
        int fd = app_open(out, LUME_WRONLY | LUME_CREAT | LUME_EXCL | LUME_DEFER_SYNC);
        if (fd >= 0)
            return fd;
        if (fd != -17)
            return fd;
    }
    return -17;
}

static void upload(int socket, const char *path, uint32_t size, uint32_t expected)
{
    if (!writable(path))
    {
        answer(socket, -LUME_NET_EACCES, 0, 0);
        return;
    }
    char temp[NET_PATH_MAX + 1];
    int fd = temporary(path, temp);
    if (fd < 0)
    {
        answer(socket, fd, 0, 0);
        return;
    }
    int status = 0, verified = 0;
    uint32_t left = size, crc = ~0u;
    if (!answer(socket, 100, size, expected))
        status = -LUME_NET_EIO;
    while (!status && left)
    {
        unsigned n = left > sizeof block ? sizeof block : left;
        if (!stream(socket, block, n, 0))
        {
            status = -LUME_NET_EIO;
            break;
        }
        unsigned done = 0;
        while (done < n)
        {
            int r = app_write(fd, block + done, n - done);
            if (r <= 0)
            {
                status = r < 0 ? r : -LUME_NET_EIO;
                break;
            }
            done += (unsigned)r;
        }
        crc = net_crc(crc, block, n);
        left -= n;
    }
    crc = ~crc;
    if (!status && crc != expected)
        status = -LUME_NET_EIO;
    if (!status)
        status = app_sync(fd);
    int closed = app_close(fd);
    if (!status && closed < 0)
        status = closed;
    /* Verify what the filesystem returns, not just the bytes from TCP. */
    if (!status)
    {
        fd = app_open(temp, LUME_RDONLY);
        if (fd < 0)
            status = fd;
        else
        {
            left = size;
            uint32_t check = ~0u, last = net_now();
            if (!answer(socket, 101, 0, 0))
                status = -LUME_NET_EIO;
            while (left && !status)
            {
                unsigned n = left > sizeof block ? sizeof block : left;
                int r = app_read(fd, block, n);
                if (r <= 0)
                {
                    status = -LUME_NET_EIO;
                    break;
                }
                check = net_crc(check, block, (unsigned)r);
                left -= (unsigned)r;
                if (net_now() - last >= 5000u)
                {
                    if (!answer(socket, 101, size - left, 0))
                        status = -LUME_NET_EIO;
                    last = net_now();
                }
            }
            if (!status && ~check != expected)
                status = -LUME_NET_EIO;
            struct lume_stat st;
            if (!status && (app_stat(temp, &st) < 0 || st.size != size))
                status = -LUME_NET_EIO;
            closed = app_close(fd);
            if (!status && closed < 0)
                status = closed;
        }
    }
    if (!status)
        verified = 1;
    if (!status)
    {
        struct lume_stat st;
        int r = app_stat(path, &st);
        if (!r)
            status = unlink_path(path);
        else if (r != -2)
            status = r;
        if (!status)
            status = rename_path(temp, path);
    }
    /* On publication failure retain the verified stage for cable recovery.
     * Failed/incomplete uploads can be removed without touching the target. */
    if (status && !verified)
        unlink_path(temp);
    answer(socket, status, size, crc);
    app_puts(status ? "netd: upload failed " : "netd: installed ");
    app_puts(path);
    app_puts("\n");
    if (status && verified)
    {
        app_puts("netd: verified stage ");
        app_puts(temp);
        app_puts("\n");
    }
}

static void download(int socket, const char *path)
{
    struct lume_stat st;
    int r = app_stat(path, &st);
    if (r < 0)
    {
        answer(socket, r, 0, 0);
        return;
    }
    if (st.size > NET_FILE_MAX)
    {
        answer(socket, -LUME_NET_EMSGSIZE, 0, 0);
        return;
    }
    int fd = app_open(path, LUME_RDONLY);
    if (fd < 0)
    {
        answer(socket, fd, 0, 0);
        return;
    }
    uint32_t left = st.size, crc = ~0u;
    int status = 0;
    if (!answer(socket, 100, st.size, 0))
        status = -LUME_NET_EIO;
    while (left && !status)
    {
        unsigned n = left > sizeof block ? sizeof block : left;
        r = app_read(fd, block, n);
        if (r <= 0)
        {
            status = -LUME_NET_EIO;
            break;
        }
        crc = net_crc(crc, block, (unsigned)r);
        if (!stream(socket, block, (unsigned)r, 1))
        {
            status = -LUME_NET_EIO;
            break;
        }
        left -= (unsigned)r;
    }
    r = app_close(fd);
    if (!status && r < 0)
        status = r;
    if (!left)
        answer(socket, status, st.size, ~crc);
}

static void serve(int fd)
{
    unsigned char header[32];
    char path[NET_PATH_MAX + 1];
    if (!stream(fd, header, sizeof header, 0))
        return;
    unsigned length = net_get32(header + 8);
    uint32_t size = net_get32(header + 12), crc = net_get32(header + 16);
    int valid = !memcmp(header, "LNET", 4) && header[4] == 1 &&
                (header[5] == 1 || header[5] == 2) && !header[6] && !header[7];
    for (unsigned i = 20; i < 32; i++)
        if (header[i])
            valid = 0;
    if (!valid || !length || length > NET_PATH_MAX || size > NET_FILE_MAX ||
        (header[5] == 2 && (size || crc)))
    {
        answer(fd, -LUME_NET_EINVAL, 0, 0);
        return;
    }
    if (!stream(fd, path, length, 0))
        return;
    path[length] = 0;
    if (memchr(path, 0, length) || !net_valid_path(path))
    {
        answer(fd, -LUME_NET_EINVAL, 0, 0);
        return;
    }
    if (header[5] == 1)
        upload(fd, path, size, crc);
    else
        download(fd, path);
}

struct dhcp_transport
{
    int fd;
};

static int dhcp_send(void *context, const unsigned char *ip, unsigned port,
                     const unsigned char *data, unsigned size)
{
    struct dhcp_transport *t = context;
    struct lume_net_request r = net_request(t->fd);
    memcpy(r.address, ip, 4);
    r.port = port;
    r.buffer = (uint32_t)(uintptr_t)data;
    r.length = size;
    uint32_t start = net_now();
    int result;
    do
    {
        result = net_call(LUME_NET_SENDTO, &r);
        if (result == -LUME_NET_EAGAIN)
            net_idle(250);
    } while (result == -LUME_NET_EAGAIN && net_now() - start < 4000u);
    return result == (int)size;
}

static int dhcp_receive(void *context, unsigned char *ip, unsigned *port, unsigned char *data,
                        unsigned size)
{
    struct dhcp_transport *t = context;
    struct lume_net_request r = net_request(t->fd);
    r.buffer = (uint32_t)(uintptr_t)data;
    r.length = size;
    int n = net_call(LUME_NET_RECVFROM, &r);
    if (n == -LUME_NET_EAGAIN || n == -LUME_NET_EACCES || n == -LUME_NET_EMSGSIZE)
        return 0;
    if (n > 0)
    {
        memcpy(ip, r.address, 4);
        *port = r.port;
    }
    return n;
}

static uint32_t dhcp_clock(void *unused)
{
    (void)unused;
    return net_now();
}

static int dhcp_idle(void *unused)
{
    (void)unused;
    net_idle(250);
    return 1;
}

static int acquire(void)
{
    memset(&configuration, 0, sizeof configuration);
    configuration.version = LUME_NET_VERSION;
    configuration.bytes = sizeof configuration;
    const unsigned char mac[6] = {2, 0x58, 0x32, 0x25, 3, 0x17};
    memcpy(configuration.mac, mac, 6);
    int result = net_config(&configuration);
    if (result < 0)
    {
        app_error("netd config", result);
        return 0;
    }
    struct dhcp_transport transport = {net_socket(LUME_NET_UDP)};
    if (transport.fd < 0)
    {
        app_error("netd DHCP socket", transport.fd);
        return 0;
    }
    if (net_bind(transport.fd, 68) < 0)
    {
        net_close(transport.fd);
        return 0;
    }
    struct lume_dhcp_io io = {&transport, dhcp_send, dhcp_receive, dhcp_clock, dhcp_idle, 0};
    struct lume_dhcp_lease lease;
    uint32_t xid = net_now() ^ 0x58324e31u;
    if (!xid)
        xid = 1;
    lume_dhcp_acquire(&io, mac, xid, &lease);
    net_close(transport.fd);
    if (lease.status != LUME_DHCP_BOUND)
    {
        app_puts("netd: DHCP status ");
        app_number(lease.status);
        app_puts(" ignored ");
        app_number(lease.ignored);
        app_puts("\n");
        return 0;
    }
    if (lease.ip[0] != 172 || lease.ip[1] != 30 || lease.ip[2] != 1 || !lease.ip[3] ||
        lease.ip[3] == 255 || lease.mask[0] != 255 || lease.mask[1] != 255 ||
        lease.mask[2] != 255 || lease.mask[3])
    {
        app_puts("netd: DHCP outside trusted172.30.1/24 rejected\n");
        return 0;
    }
    memcpy(configuration.address, lease.ip, 4);
    memcpy(configuration.netmask, lease.mask, 4);
    memcpy(configuration.gateway, lease.gateway, 4);
    memcpy(configuration.dns, lease.dns, 4);
    if (net_config(&configuration) < 0)
        return 0;
    /* Reacquire at half-lease and clear the address on failure.
     * Reacquisition may briefly disconnect clients. */
    unsigned seconds = lease.seconds > 172800u ? 172800u : lease.seconds;
    renewal = net_now() + (seconds > 1 ? seconds * 500u : 500u);
    expiry = net_now() + seconds * 1000u;
    app_puts("NET_READY IP=");
    net_ip(configuration.address);
    app_puts(" TCP=8022 LEASE=");
    app_number(lease.seconds);
    app_puts("\n");
    return 1;
}

int app_main(void)
{
    app_puts("netd: W5500 DHCP / trusted172.30.1.0/24\n");
    sequence = net_now() & 0xfffffu;
    for (;;)
    {
        if (!acquire())
        {
            net_idle(3000);
            continue;
        }
        int listener = net_socket(LUME_NET_TCP);
        if (listener < 0 || net_bind(listener, NET_PORT) < 0 || net_listen(listener) < 0)
        {
            if (listener >= 0)
                net_close(listener);
            net_idle(3000);
            continue;
        }
        while ((int32_t)(net_now() - renewal) < 0)
        {
            struct lume_net_request r = net_request(listener);
            int fd = net_call(LUME_NET_ACCEPT, &r);
            if (fd >= 0)
            {
                serve(fd);
                close_wait(fd);
            }
            else if (fd == -LUME_NET_EACCES || fd == -LUME_NET_EMFILE)
                net_idle(250);
            else if (fd != -LUME_NET_EAGAIN)
            {
                app_error("netd accept", fd);
                break;
            }
            else
                net_idle(250);
        }
        close_wait(listener);
    }
}
