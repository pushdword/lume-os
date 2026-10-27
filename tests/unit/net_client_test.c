#define LUME_USER_HOST_TEST 1
#define puts user_puts
#include "lume/app_support.h"
#include "net_client.h"
#undef puts
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks, dhcp_index, admin_index, service_calls, service_command_id;
static struct lume_netif_info interfaces[2];
static uint64_t sequence;
static char actual_output[4096];
static unsigned output_bytes, write_calls;

#define T(x)                                                                                       \
    do                                                                                             \
    {                                                                                              \
        ++checks;                                                                                  \
        if (!(x))                                                                                  \
        {                                                                                          \
            fprintf(stderr, "NET_CLIENT FAIL %d: %s\n", __LINE__, #x);                             \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

int64_t call(uint64_t number, uint64_t a, uint64_t b, uint64_t c)
{
    (void)c;
    if (number == LUME_SYS_NETCTL)
    {
        struct lume_net_control *r = (void *)(uintptr_t)b;
        if (a == LUME_NET_ENUM)
        {
            r->count = 2;
            memcpy(r->data.interfaces, interfaces, sizeof(interfaces));
            return 0;
        }
        unsigned i = r->ifindex - 1;
        if (i >= 2)
            return -LUME_ENOENT;
        if (a == LUME_NET_GET)
            r->data.interfaces[0] = interfaces[i];
        else if (a == LUME_NET_DHCP_START)
            dhcp_index = r->ifindex;
        else if (a == LUME_NET_ADMIN)
            admin_index = r->ifindex;
        else if (a == LUME_NET_WIFI_CAPS || a == LUME_NET_WIFI_SCAN)
            return interfaces[i].kind == LUME_IF_WIFI ? 0 : -LUME_EINVAL;
        else
            return -LUME_EINVAL;
        return 0;
    }
    if (number == LUME_SYS_SERVICE)
    {
        if (a == LUME_SERVICE_GET)
        {
            *(struct lume_net_service_status *)(uintptr_t)b =
                (struct lume_net_service_status){.version = 1, .acknowledged = sequence};
            return 0;
        }
        if (a == LUME_SERVICE_SEND)
        {
            struct lume_service_message *m = (void *)(uintptr_t)b;
            ++service_calls;
            service_command_id = m->command;
            sequence = m->sequence;
            return 0;
        }
    }
    if (number == LUME_SYS_WRITE)
    {
        if (++write_calls % 7 == 1)
            return -4;
        if (c > 3)
            c = 3;
        T(a == 1 && c < sizeof(actual_output) - output_bytes);
        memcpy(actual_output + output_bytes, (void *)(uintptr_t)b, c);
        output_bytes += (unsigned)c;
        actual_output[output_bytes] = 0;
        return (int64_t)c;
    }
    if (number == LUME_SYS_SLEEP)
        return 0;
    return -LUME_ENOSYS;
}

static int run(const char *verb, const char *name)
{
    char *args[] = {"net", (char *)verb, (char *)name};
    net_used = 0;
    net_output[0] = 0;
    output_bytes = write_calls = 0;
    actual_output[0] = 0;
    int result = network(args, !verb ? 1 : name ? 3 : 2);
    T(output_bytes == net_used && !memcmp(actual_output, net_output, net_used));
    return result;
}

int main(void)
{
    interfaces[0] = (struct lume_netif_info){
        .ifindex = 1, .kind = LUME_IF_WIFI, .name = "wlan0", .flags = LUME_IF_UP};
    interfaces[1] = (struct lume_netif_info){
        .ifindex = 2, .kind = LUME_IF_ETHERNET, .name = "eth0", .flags = LUME_IF_UP | LUME_IF_LINK};
    T(run("status", 0) == 0);
    T(strstr(net_output, "wlan0") && strstr(net_output, "eth0"));
    T(run(0, 0) == 0 && strstr(net_output, "eth0"));
    T(run("status", "eth0") == 0);
    T(strstr(net_output, "eth0") && !strstr(net_output, "wlan0"));
    T(run("status", "absent") == 1);
    T(run("dhcp", 0) == 0 && dhcp_index == 2);
    T(run("dhcp", "wlan0") == 0 && dhcp_index == 1);
    T(run("caps", 0) == 0);
    T(run("down", "eth0") == 0 && admin_index == 2 && service_calls == 0);
    T(run("up", "eth0") == 0 && admin_index == 2 && service_calls == 0);
    T(run("down", "wlan0") == 0 && admin_index == 1);
    T(service_calls == 1 && service_command_id == NETD_PAUSE);
    T(run("up", "wlan0") == 0 && admin_index == 1);
    T(service_calls == 2 && service_command_id == NETD_RECONNECT);
    printf("PASS_NET_CLIENT checks=%u\n", checks);
    return 0;
}
