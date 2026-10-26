/* Shell client of the typed ABI. Profile policy/formatting live in EL0. */
#include "lume/net_abi.h"
#include <stdbool.h>
static struct lume_net_control net_request;
static uint64_t net_cursor;
static char net_output[4096];
static size_t net_used;

static void net_text(const char *s)
{
    while (*s && net_used + 1 < sizeof(net_output))
        net_output[net_used++] = *s++;
    net_output[net_used] = 0;
}

static void net_zero(void *p, size_t n)
{
    volatile unsigned char *b = p;
    while (n--)
        *b++ = 0;
}

static void net_init(uint32_t index)
{
    net_zero(&net_request, sizeof(net_request));
    net_request.version = LUME_NET_CONTROL_VERSION;
    net_request.ifindex = index;
}

static int64_t net_call(unsigned op)
{
    return call(LUME_SYS_NETCTL, op, (uintptr_t)&net_request, sizeof(net_request));
}

static void net_number(uint64_t n)
{
    char b[21];
    unsigned i = 0;
    do
    {
        b[i++] = (char)('0' + n % 10);
        n /= 10;
    } while (n);
    while (i)
    {
        char s[2] = {b[--i], 0};
        net_text(s);
    }
}

static void net_address(const uint8_t ip[4])
{
    for (unsigned i = 0; i < 4; ++i)
    {
        if (i)
            net_text(".");
        net_number(ip[i]);
    }
}

static void net_mac(const uint8_t mac[6])
{
    const char *h = "0123456789abcdef";
    for (unsigned i = 0; i < 6; ++i)
    {
        char b[4] = {h[mac[i] >> 4], h[mac[i] & 15], i == 5 ? 0 : ':', 0};
        net_text(b);
    }
}

static void net_status(const struct lume_netif_info *n)
{
    net_text(n->name);
    net_text("  ");
    net_mac(n->mac);
    net_text((n->flags & LUME_IF_LINK)
                 ? (n->kind == LUME_IF_WIFI ? "  associated\n" : "  link-up\n")
                 : "  disconnected\n");
    net_text("inet ");
    net_address(n->address);
    net_text(" mask ");
    net_address(n->netmask);
    net_text(" gateway ");
    net_address(n->gateway);
    net_text("\ndns ");
    net_address(n->dns);
    net_text("\nRX ");
    net_number(n->rx_packets);
    net_text(" TX ");
    net_number(n->tx_packets);
    net_text(" RX-errors ");
    net_number(n->rx_errors);
    net_text(" TX-errors ");
    net_number(n->tx_errors);
    net_text("\n");
}

#include "net_service_client.h"

static int network_run(char **args, unsigned count)
{
    int managed = service_command(args, count);
    if (managed >= 0)
        return managed;
    static const char *const verbs[] = {"status", "scan",   "join", "leave",  "dhcp", "ping", "dns",
                                        "tcp",    "ifaces", "caps", "events", "up",   "down"};
    unsigned op = 0;
    if (count > 1)
        for (; op < 13; ++op)
            if (eq(args[1], verbs[op]))
                break;
    bool diagnostic = op >= 5 && op <= 7;
    if (op == 13 || count > 3 || (diagnostic && count != 3) || (op == 8 && count > 2))
    {
        net_text(
            "net status|scan|join|leave|dhcp|ifaces|caps|events|up|down [INTERFACE]\nnet ping HOST|dns HOST|tcp HOST:PORT\n");
        return 1;
    }
    net_init(0);
    if (net_call(LUME_NET_ENUM) < 0)
    {
        net_text("net: interface query failed\n");
        return 1;
    }
    uint32_t index = 0, kind = 0;
    unsigned best = 0;
    bool all_status = op == 0 && count < 3;
    for (unsigned i = 0; i < net_request.count; ++i)
    {
        struct lume_netif_info *n = &net_request.data.interfaces[i];
        if (all_status)
            net_status(n);
        if (op == 8)
        {
            net_number(n->ifindex);
            net_text(" ");
            net_text(n->name);
            net_text(n->kind == LUME_IF_WIFI ? " wifi mtu " : " ethernet mtu ");
            net_number(n->mtu);
            net_text((n->flags & LUME_IF_UP) ? " up\n" : " down\n");
        }
        if (!diagnostic && count == 3)
        {
            if (eq(args[2], n->name))
            {
                index = n->ifindex;
                kind = n->kind;
            }
        }
        else
        {
            if ((op == 1 || op == 9) && n->kind != LUME_IF_WIFI)
                continue;
            unsigned score = 1 + ((n->flags & LUME_IF_UP) ? 1 : 0) +
                             ((n->flags & LUME_IF_LINK) ? 2 : 0) +
                             ((n->flags & LUME_IF_DHCP) ? 4 : 0);
            if (score > best)
            {
                best = score;
                index = n->ifindex;
                kind = n->kind;
            }
        }
    }
    if (op == 8 || all_status)
        return 0;
    if (!index)
    {
        net_text("net: interface unavailable\n");
        return 1;
    }
    if (diagnostic)
    {
        struct lume_net_request legacy;
        net_zero(&legacy, sizeof(legacy));
        size_t n = len(args[2]);
        if (n >= sizeof(legacy.argument))
            return 1;
        for (size_t i = 0; i <= n; ++i)
            legacy.argument[i] = args[2][i];
        int64_t result = call(LUME_SYS_NET, op, (uintptr_t)&legacy, sizeof(legacy));
        net_text(legacy.output);
        if (result < 0)
        {
            net_text("net: operation failed\n");
            return 1;
        }
        return 0;
    }
    if (op == 12 && kind == LUME_IF_WIFI && service_send(NETD_PAUSE, 0))
        return 1;
    net_init(index);
    int64_t result = 0;
    if (op == 0)
    {
        result = net_call(LUME_NET_GET);
        if (!result)
            net_status(&net_request.data.interfaces[0]);
    }
    else if (op == 1)
    {
        result = net_call(LUME_NET_WIFI_SCAN);
        if (!result)
            for (unsigned i = 0; i < net_request.count; ++i)
            {
                const struct lume_wifi_bss_info *a = &net_request.data.scan[i];
                net_mac(a->bssid);
                net_text(" ch ");
                net_number(a->channel);
                net_text(" -");
                net_number(a->rssi < 0 ? (unsigned)-a->rssi : (unsigned)a->rssi);
                net_text("dBm ");
                net_text((a->security & LUME_BSS_RSN)   ? "RSN "
                         : (a->security & LUME_BSS_WPA) ? "WPA "
                         : a->security                  ? "privacy "
                                                        : "open ");
                for (unsigned k = 0; k < a->ssid_bytes; ++k)
                {
                    char b[2] = {a->ssid[k] >= 32 && a->ssid[k] < 127 ? (char)a->ssid[k] : '?', 0};
                    net_text(b);
                }
                net_text("\n");
            }
    }
    else if (op == 2 || op == 3)
    {
        result = -LUME_EINVAL;
    }
    else if (op == 4)
    {
        result = net_call(LUME_NET_DHCP_START);
        if (!result)
            net_text("DHCP ready\n");
    }
    else if (op == 9)
    {
        result = net_call(LUME_NET_WIFI_CAPS);
        if (!result)
        {
            const struct lume_wifi_caps *c = &net_request.data.capabilities;
            net_text("station scan WPA2-CCMP bands");
            if (c->bands & 1)
                net_text(" 2.4GHz");
            if (c->bands & 2)
                net_text(" 5GHz");
            char country[3] = {c->country[0], c->country[1], 0};
            net_text(" country ");
            net_text(country);
            net_text("\n");
        }
    }
    else if (op == 10)
    {
        for (unsigned page = 0; page < 3; ++page)
        {
            net_init(count == 3 ? index : 0);
            net_request.cursor = net_cursor;
            result = net_call(LUME_NET_EVENTS);
            if (result < 0)
                break;
            net_cursor = net_request.cursor;
            net_text("events ");
            net_number(net_request.count);
            net_text(" lost ");
            net_number(net_request.lost);
            net_text("\n");
            for (unsigned i = 0; i < net_request.count; ++i)
            {
                const struct lume_net_event *e = &net_request.data.events[i];
                net_text("event ");
                net_number(e->sequence);
                net_text(" if ");
                net_number(e->ifindex);
                net_text(" type ");
                net_number(e->type);
                net_text(" state ");
                net_number(e->state);
                net_text(" reason ");
                net_number(e->reason);
                net_text("\n");
            }
            if (net_request.count < LUME_NET_EVENTS_MAX)
                break;
        }
    }
    else if (op == 11 || op == 12)
    {
        net_request.data.admin_up = op == 11;
        result = net_call(LUME_NET_ADMIN);
        if (!result)
            net_text(op == 11 ? "interface up\n" : "interface down\n");
    }
    if (result < 0)
    {
        net_text("net: operation failed\n");
        return 1;
    }
    if (op == 11 && kind == LUME_IF_WIFI)
        return service_send(NETD_RECONNECT, 0);
    return 0;
}

static int network(char **args, unsigned count)
{
    net_used = 0;
    int result = network_run(args, count);
    if (net_used)
        (void)write(1, net_output, net_used);
    return result;
}
