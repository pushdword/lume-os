#include "lume/abi.h"
#include "lume/net_commands.h"
#include <stddef.h>

static struct lume_net_control request;
static struct lume_net_request packet;

static int64_t invoke(uint64_t number, uint64_t a, uint64_t b, uint64_t c)
{
    register uint64_t x8 __asm__("x8") = number, x0 __asm__("x0") = a;
    register uint64_t x1 __asm__("x1") = b, x2 __asm__("x2") = c;
    __asm__ volatile("svc #0x4c55" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2) : "memory", "cc");
    return (int64_t)x0;
}

static void text(const char *value)
{
    size_t bytes = 0;
    while (value[bytes])
        ++bytes;
    (void)invoke(LUME_SYS_WRITE, 1, (uintptr_t)value, bytes);
}

static void number(uint64_t value)
{
    const char digits[] = "0123456789abcdef";
    char output[17];
    for (unsigned i = 0; i < 16; ++i)
        output[i] = digits[(value >> (60 - 4 * i)) & 15];
    output[16] = 0;
    text(output);
}

static void address(const unsigned char ip[4])
{
    char output[16];
    unsigned used = 0;
    for (unsigned i = 0; i < 4; ++i)
    {
        unsigned n = ip[i];
        if (i)
            output[used++] = '.';
        if (n >= 100)
            output[used++] = (char)('0' + n / 100);
        if (n >= 10)
            output[used++] = (char)('0' + n / 10 % 10);
        output[used++] = (char)('0' + n % 10);
    }
    output[used] = 0;
    text(output);
}

static int control(unsigned operation, unsigned index)
{
    request = (struct lume_net_control){.version = LUME_NET_CONTROL_VERSION, .ifindex = index};
    return (int)invoke(LUME_SYS_NETCTL, operation, (uintptr_t)&request, sizeof(request));
}

static int fail(unsigned stage, int result)
{
    text("GEM_NETIF_PROBE_FAIL stage=");
    number(stage);
    text(" result=");
    number((uint64_t)(int64_t)result);
    text("\n");
    return 1;
}

int main(int argc, char **argv)
{
    int result = control(LUME_NET_ENUM, 0);
    if (result)
        return fail(1, result);
    unsigned index = 0, count = request.count;
    for (unsigned i = 0; i < count; ++i)
        if (request.data.interfaces[i].kind == LUME_IF_ETHERNET)
            index = request.data.interfaces[i].ifindex;
    if (!index)
        return fail(2, -LUME_ENOENT);
    if ((result = control(LUME_NET_GET, index)) != 0)
        return fail(3, result);
    text("GEM_NETIF_ENUM_PASS index=");
    number(index);
    text(" flags=");
    number(request.data.interfaces[0].flags);
    text("\n");
    if (argc > 1 && argv[1][0] == 'i')
        return 0;
    if (argc > 1 && argv[1][0] == 'c')
    {
        control(LUME_NET_GET, index);
        request.data.admin_up = 0;
        result = (int)invoke(LUME_SYS_NETCTL, LUME_NET_ADMIN, (uintptr_t)&request, sizeof(request));
        if (result || request.data.interfaces[0].flags)
            return fail(4, result);
        control(LUME_NET_GET, index);
        request.data.admin_up = 1;
        result = (int)invoke(LUME_SYS_NETCTL, LUME_NET_ADMIN, (uintptr_t)&request, sizeof(request));
        if (result)
            return fail(5, result);
        if (control(LUME_NET_ENUM, 0) || request.count != count)
            return fail(6, -LUME_EIO);
        text("GEM_NETIF_ADMIN_CYCLE_PASS\n");
    }
    if ((result = control(LUME_NET_DHCP_START, index)) != 0)
        return fail(7, result);
    struct lume_netif_info before = request.data.interfaces[0];
    if (!(before.flags & LUME_IF_DHCP) || !before.address[0])
        return fail(8, -LUME_EIO);
    text("GEM_NETIF_DHCP_PASS address=");
    address(before.address);
    text(" gateway=");
    address(before.gateway);
    text("\n");
    unsigned used = 0;
    for (unsigned i = 0; i < 4; ++i)
    {
        unsigned n = before.gateway[i];
        if (i)
            packet.argument[used++] = '.';
        if (n >= 100)
            packet.argument[used++] = (char)('0' + n / 100);
        if (n >= 10)
            packet.argument[used++] = (char)('0' + n / 10 % 10);
        packet.argument[used++] = (char)('0' + n % 10);
    }
    packet.argument[used] = 0;
    result = (int)invoke(LUME_SYS_NET, LUME_NET_PING, (uintptr_t)&packet, sizeof(packet));
    if (result < 0)
        return fail(9, result);
    if (control(LUME_NET_GET, index))
        return fail(10, -LUME_EIO);
    struct lume_netif_info after = request.data.interfaces[0];
    if (after.tx_packets <= before.tx_packets || after.rx_packets <= before.rx_packets)
        return fail(11, -LUME_EIO);
    text("GEM_NETIF_PING_PASS\nGEM_NETIF_USER_ABI_PASS\n");
    return 0;
}
