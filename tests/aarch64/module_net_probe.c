#include "lib.h"
#include "lume/hardware_abi.h"
#include "lume/module_abi.h"
#include "lume/io.h"

int main(int argc, char **argv)
{
    struct lume_net_control network = {.version = LUME_NET_CONTROL_VERSION};
    if (call(LUME_SYS_NETCTL, LUME_NET_ENUM, (uintptr_t)&network, sizeof(network)))
        return 1;
    unsigned index = 0;
    for (unsigned i = 0; i < network.count; ++i)
        if (network.data.interfaces[i].kind == LUME_IF_ETHERNET)
            index = network.data.interfaces[i].ifindex;
    if (!index)
        return 2;
    network = (struct lume_net_control){.version = LUME_NET_CONTROL_VERSION, .ifindex = index};
    if (call(LUME_SYS_NETCTL, LUME_NET_DHCP_START, (uintptr_t)&network, sizeof(network)))
        return 3;
    const struct lume_netif_info *info = network.data.interfaces;
    if (info->address[0] != 10 || info->address[1] != 0 || info->address[2] != 3 ||
        info->address[3] != 15 || !info->tx_packets || !info->rx_packets)
        return 4;
    puts("PASS_MODULE_NET DHCP=10.0.3.15 TX=active RX=active\n");
    if (argc < 2 || !eq(argv[1], "busy"))
        return 0;
    struct lume_hardware_request hardware = {.version = LUME_HARDWARE_VERSION,
                                             .operation = LUME_HARDWARE_LIST};
    unsigned hardware_id = 0;
    while (!call(LUME_SYS_HARDWARE, (uintptr_t)&hardware, sizeof(hardware), 0))
    {
        if (eq(hardware.info.driver, "gem"))
            hardware_id = hardware.info.id;
        hardware.index++;
    }
    if (!hardware_id)
        return 5;
    struct lume_stream_request stream = {.version = LUME_IO_VERSION, .port = 23145};
    if (call(LUME_SYS_STREAM, LUME_DATAGRAM_LISTEN, (uintptr_t)&stream, sizeof(stream)))
        return 6;
    hardware = (struct lume_hardware_request){
        .version = LUME_HARDWARE_VERSION, .operation = LUME_HARDWARE_UNBIND, .id = hardware_id};
    if (call(LUME_SYS_HARDWARE, (uintptr_t)&hardware, sizeof(hardware), 0) != -LUME_EBUSY)
        return 7;
    if (call(LUME_SYS_STREAM, LUME_STREAM_CLOSE, (uintptr_t)&stream, sizeof(stream)))
        return 8;
    puts("PASS_MODULE_NET socket-unbind=busy stop-state=retained\n");
    return 0;
}
