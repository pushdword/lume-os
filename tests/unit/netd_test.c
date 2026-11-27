/* Execute the actual manager with only syscall boundaries replaced. */
#define LUME_USER_HOST_TEST 1
#define main manager_main
#define puts user_puts
#include "netd.c"
#undef puts
#undef main
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks, connections, dhcps, drops;
static uint64_t milliseconds;
static bool connection_fail, dhcp_fail, write_fail, admin = true, link, address;
static bool connection_pending, dhcp_pending;
static bool ethernet_present, ethernet_admin, ethernet_link, ethernet_address;
static bool ethernet_pending, ethernet_fail;
static unsigned ethernet_dhcps;
static bool test_loop, old_wait;
static unsigned pending_commands, receives, acknowledgements, event_waits, sleeps;
static uint64_t command_sequence, published_sequence;
static bool test_clock, utc_valid, time_reply, wrong_nonce;
static unsigned time_connects, time_sends, time_closes, time_sets;
static unsigned char time_nonce[8];

static struct
{
    bool exists;
    unsigned size;
    uint8_t data[NET_STORE_BYTES];
} files[4];

static struct
{
    unsigned file, offset;
    bool used;
} descriptors[8];

#define T(x)                                                                                       \
    do                                                                                             \
    {                                                                                              \
        ++checks;                                                                                  \
        if (!(x))                                                                                  \
        {                                                                                          \
            fprintf(stderr, "NETD FAIL %d: %s\n", __LINE__, #x);                                   \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

int64_t call(uint64_t number, uint64_t a, uint64_t b, uint64_t c)
{
    if (number == LUME_SYS_CLOCK)
        return (int64_t)milliseconds;
    if (number == LUME_SYS_GETPID)
        return 3;
    if (number == LUME_SYS_REALTIME)
    {
        if (!test_clock)
            return -LUME_ENOSYS;
        struct lume_realtime_request *utc = (void *)(uintptr_t)b;
        T(utc->version == LUME_REALTIME_VERSION && utc->bytes == sizeof(*utc) &&
          c == sizeof(*utc));
        if (a == LUME_REALTIME_GET)
            return utc_valid ? 0 : -LUME_EAGAIN;
        T(a == LUME_REALTIME_INITIALIZE && !utc_valid && utc->seconds == 1793628000 &&
          !utc->nanoseconds);
        ++time_sets;
        utc_valid = true;
        return 0;
    }
    if (number == LUME_SYS_RANDOM && test_clock)
    {
        T(b == sizeof(time_nonce) && !c);
        memset((void *)(uintptr_t)a, 0x42, b);
        return (int64_t)b;
    }
    if (number == LUME_SYS_STREAM && test_clock)
    {
        struct lume_stream_request *packet = (void *)(uintptr_t)b;
        T(packet->version == LUME_IO_VERSION && c == sizeof(*packet));
        if (a == LUME_DATAGRAM_CONNECT)
        {
            T(packet->port == 123 && packet->address[0] == 162);
            T(packet->address[3] == (time_connects == 1 ? 123 : 1));
            ++time_connects;
            packet->handle = 19;
            return 0;
        }
        T(packet->handle == 19);
        if (a == LUME_STREAM_SEND)
        {
            T(packet->count == 48 && packet->data[0] == 0x23);
            memcpy(time_nonce, packet->data + 40, sizeof(time_nonce));
            ++time_sends;
            return (int64_t)packet->count;
        }
        if (a == LUME_STREAM_POLL)
        {
            packet->count = time_reply ? LUME_STREAM_READABLE : 0;
            return (int64_t)packet->count;
        }
        if (a == LUME_STREAM_RECEIVE)
        {
            T(time_reply && packet->count >= 48);
            memset(packet->data, 0, 48);
            packet->data[0] = 0x24;
            packet->data[1] = 2;
            memcpy(packet->data + 24, time_nonce, sizeof(time_nonce));
            if (wrong_nonce)
                packet->data[24] ^= 1;
            uint32_t timestamp = UINT32_C(2208988800) + UINT32_C(1793628000);
            for (unsigned i = 0; i < 4; ++i)
                packet->data[40 + i] = (unsigned char)(timestamp >> (24 - 8 * i));
            packet->count = 48;
            time_reply = false;
            return 48;
        }
        T(a == LUME_STREAM_CLOSE);
        ++time_closes;
        return 0;
    }
    if (number == LUME_SYS_FILE_MODE)
    {
        const struct lume_file_mode_request *request = (void *)(uintptr_t)b;
        T(c == sizeof(*request) && request->bytes == sizeof(*request));
        T(request->version == LUME_FILE_MODE_VERSION && request->mode == 0600);
        if (a == LUME_FILE_MODE_OPEN)
            return call(LUME_SYS_OPEN, request->path, request->flags, 0);
        T(a == LUME_FILE_MODE_FD && request->fd >= 3 && request->fd < 8);
        T(descriptors[request->fd].used);
        return 0;
    }
    if (number == LUME_SYS_UNLINK)
    {
        T(!strcmp((const char *)(uintptr_t)a, "/data/system/wifi-new.cfg"));
        if (!files[3].exists)
            return -LUME_ENOENT;
        files[3].exists = false;
        return 0;
    }
    if (number == LUME_SYS_OPEN)
    {
        int index = -1;
        for (unsigned i = 0; i < 2; ++i)
            if (!strcmp((const char *)(uintptr_t)a, stores[i]))
                index = (int)i;
        if (!strcmp((const char *)(uintptr_t)a, "/data/system/wifi.cfg"))
            index = 2;
        if (!strcmp((const char *)(uintptr_t)a, "/data/system/wifi-new.cfg"))
            index = 3;
        if (index < 0)
            return -LUME_ENOENT;
        if (!files[index].exists && !(b & LUME_O_CREAT))
            return -LUME_ENOENT;
        for (unsigned i = 3; i < 8; ++i)
            if (!descriptors[i].used)
            {
                files[index].exists = true;
                if (b & LUME_O_TRUNC)
                    files[index].size = 0;
                descriptors[i].used = true;
                descriptors[i].file = (unsigned)index;
                descriptors[i].offset = 0;
                return i;
            }
        return -LUME_EMFILE;
    }
    if (number == LUME_SYS_STAT)
    {
        struct lume_stat *st = (void *)(uintptr_t)b;
        if (a >= 8 || !descriptors[a].used)
            return -LUME_EBADF;
        *st = (struct lume_stat){.size = files[descriptors[a].file].size, .kind = 1};
        return 0;
    }
    if (number == LUME_SYS_CLOSE)
    {
        if (a >= 8 || !descriptors[a].used)
            return -LUME_EBADF;
        descriptors[a].used = false;
        return 0;
    }
    if (number == LUME_SYS_READ || number == LUME_SYS_WRITE)
    {
        if (a < 3)
            return number == LUME_SYS_WRITE ? (int64_t)c : -LUME_EAGAIN;
        if (a >= 8 || !descriptors[a].used)
            return -LUME_EBADF;
        unsigned f = descriptors[a].file, off = descriptors[a].offset, n = (unsigned)c;
        if (n > 37)
            n = 37; /* real short I/O */
        if (number == LUME_SYS_READ)
        {
            if (n > files[f].size - off)
                n = files[f].size - off;
            memcpy((void *)(uintptr_t)b, files[f].data + off, n);
        }
        else
        {
            if (write_fail && off >= 74)
                return -LUME_EIO;
            if (n > NET_STORE_BYTES - off)
                return -LUME_ENOSPC;
            memcpy(files[f].data + off, (void *)(uintptr_t)b, n);
            if (off + n > files[f].size)
                files[f].size = off + n;
        }
        descriptors[a].offset += n;
        return n;
    }
    if (number == LUME_SYS_SERVICE)
    {
        if (!test_loop)
            return a == LUME_SERVICE_RECEIVE ? -LUME_EAGAIN : 0;
        if (a == LUME_SERVICE_RECEIVE)
        {
            ++receives;
            if (!pending_commands)
                return -LUME_EAGAIN;
            --pending_commands;
            *(struct lume_service_message *)(uintptr_t)b = (struct lume_service_message){
                .version = 1, .sequence = ++command_sequence, .command = NETD_PAUSE};
            return 0;
        }
        if (a == LUME_SERVICE_PUBLISH)
        {
            struct lume_net_service_status *published = (void *)(uintptr_t)b;
            if (published->acknowledged != published_sequence)
            {
                T(published->acknowledged == published_sequence + 1);
                published_sequence = published->acknowledged;
                ++acknowledgements;
            }
            return 0;
        }
        T(a == LUME_SERVICE_WAIT);
        struct lume_service_wait *wait = (void *)(uintptr_t)b;
        T(wait->version == 1 && wait->bytes == sizeof(*wait) && !wait->sequence &&
          wait->timeout_ms == 250);
        ++event_waits;
        return old_wait ? -LUME_EINVAL : 0;
    }
    if (number == LUME_SYS_SLEEP)
    {
        if (test_loop)
            ++sleeps;
        milliseconds += a;
        return 0;
    }
    if (number == LUME_SYS_NETCTL)
    {
        struct lume_net_control *r = (void *)(uintptr_t)b;
        struct lume_netif_info eth = {.ifindex = 2,
                                      .kind = LUME_IF_ETHERNET,
                                      .name = "eth0",
                                      .flags = (ethernet_admin ? LUME_IF_UP : 0) |
                                               (ethernet_link ? LUME_IF_LINK : 0) |
                                               (ethernet_address ? LUME_IF_DHCP : 0)};
        if (ethernet_address)
            eth.address[0] = 10;
        if (r->ifindex == 2)
        {
            if (a == LUME_NET_DHCP_BEGIN)
            {
                ++ethernet_dhcps;
                ethernet_address = !ethernet_fail && !ethernet_pending;
                return ethernet_fail ? -LUME_EAGAIN : 0;
            }
            if (a == LUME_NET_GET)
            {
                r->data.interfaces[0] = eth;
                return 0;
            }
            return -LUME_EINVAL;
        }
        switch (a)
        {
        case LUME_NET_ENUM:
            r->count = 1;
            r->data.interfaces[0] = (struct lume_netif_info){.ifindex = 1, .kind = LUME_IF_WIFI};
            if (ethernet_present)
                r->data.interfaces[r->count++] = eth;
            return 0;
        case LUME_NET_GET:
            r->data.interfaces[0] = (struct lume_netif_info){.ifindex = 1,
                                                             .kind = LUME_IF_WIFI,
                                                             .flags = (admin ? LUME_IF_UP : 0) |
                                                                      (link ? LUME_IF_LINK : 0) |
                                                                      (address ? LUME_IF_DHCP : 0)};
            if (address)
                r->data.interfaces[0].address[0] = 172;
            return 0;
        case LUME_NET_WIFI_CONNECT_BEGIN:
            ++connections;
            T(r->data.connect.key_bytes == 8 && !memcmp(r->data.connect.key, "password", 8));
            link = !connection_fail && !connection_pending;
            address = false;
            return connection_fail ? -LUME_EIO : 0;
        case LUME_NET_WIFI_STATE:
            r->data.wireless.state = connection_pending ? LUME_WIFI_CONNECTING
                                     : link             ? LUME_WIFI_CONNECTED
                                                        : LUME_WIFI_FAILED;
            return 0;
        case LUME_NET_DHCP_BEGIN:
            ++dhcps;
            address = !dhcp_fail && !dhcp_pending;
            return dhcp_fail ? -LUME_EAGAIN : 0;
        case LUME_NET_WIFI_DISCONNECT:
            ++drops;
            link = address = false;
            return 0;
        }
        return -LUME_EINVAL;
    }
    return -LUME_ENOSYS;
}

static void cmd(unsigned op, const char *name)
{
    struct lume_service_message m = {
        .sequence = status.acknowledged + 1, .version = 1, .command = op};
    if (name)
        memcpy(m.name, name, strlen(name) + 1);
    command(&m);
}

static void settle(uint64_t now)
{
    milliseconds = now;
    for (unsigned i = 0; i < 3; ++i)
        step(now);
}

int main(void)
{
    files[2].exists = true;
    files[2].size = 104;
    memcpy(files[2].data, "LWN1", 4);
    files[2].data[4] = 4;
    files[2].data[5] = 8;
    memcpy(files[2].data + 8, "IEEE", 4);
    memcpy(files[2].data + 40, "password", 8);
    T(load());
    T(store_slot == 0 && profiles.count == 1 && profiles.automatic && profiles.generation == 2);
    T(files[0].exists && !files[1].exists);
    settle(0);
    T(status.state == NETD_ONLINE && connections == 1 && dhcps == 1);
    settle(250);
    T(connections == 1);
    /* Spontaneous loss, delayed retry, then successful WPA + DHCP. */
    link = address = false;
    settle(1000);
    T(status.state == NETD_WAITING && status.retry_at == 2000 && connections == 1);
    settle(1999);
    T(connections == 1);
    settle(2000);
    T(status.state == NETD_ONLINE && connections == 2);
    cmd(NETD_PAUSE, 0);
    T(!status.result && paused && !link);
    settle(100000);
    T(status.state == NETD_OFF && connections == 2);
    cmd(NETD_RECONNECT, 0);
    settle(100001);
    T(status.state == NETD_ONLINE && connections == 3);
    cmd(NETD_AUTO_OFF, 0);
    T(!status.result && !profiles.automatic && profiles.generation == 3 && store_slot == 1);
    paused = false;
    force = false;
    settle(200000);
    T(status.state == NETD_OFF && connections == 3);
    T(load() && !profiles.automatic);
    cmd(NETD_AUTO_ON, 0);
    settle(200001);
    T(status.state == NETD_ONLINE && connections == 4);
    cmd(NETD_IMPORT, "backup");
    T(!status.result && profiles.count == 2);
    cmd(NETD_PREFER, "backup");
    T(!status.result && net_preferred(&profiles, 0) == 1);
    T(load() && profiles.count == 2 && net_preferred(&profiles, 0) == 1);
    cmd(NETD_CONNECT, "absent");
    T(status.result == -LUME_ENOENT);
    cmd(NETD_IMPORT, "../bad");
    T(status.result == -LUME_EINVAL);
    cmd(NETD_RECONNECT, 0);
    connection_fail = true;
    milliseconds = 300000;
    settle(milliseconds);
    T(status.state == NETD_WAITING && status.attempts == 1 && status.retry_at == 301000);
    unsigned count = connections;
    settle(300999);
    T(connections == count);
    milliseconds = 301000;
    settle(milliseconds);
    T(status.attempts == 2 && status.retry_at == 303000);
    T(chosen == -1);
    connection_fail = false;
    dhcp_fail = true;
    milliseconds = 303000;
    settle(milliseconds);
    T(status.attempts == 3 && status.retry_at == 307000 && !link);
    dhcp_fail = false;
    settle(307000);
    T(status.state == NETD_ONLINE && !status.attempts);
    admin = false;
    link = address = false;
    count = connections;
    settle(400000);
    T(status.state == NETD_OFF && connections == count);
    admin = true;
    settle(400001);
    T(status.state == NETD_ONLINE && connections == count + 1);
    /* Association and DHCP remain pending across manager iterations. Service commands
     * can cancel either stage without waiting for its deadline. */
    cmd(NETD_RECONNECT, 0);
    connection_pending = true;
    milliseconds = 500000;
    step(milliseconds);
    T(phase == CONNECT_AUTHENTICATING && status.state == NETD_CONNECTING);
    unsigned previous_dhcps = dhcps;
    count = connections;
    for (unsigned i = 1; i <= 8; ++i)
        step(milliseconds + i * 250);
    T(connections == count && dhcps == previous_dhcps);
    connection_pending = false;
    link = true;
    dhcp_pending = true;
    step(milliseconds + 2500);
    T(phase == CONNECT_DHCP && !address && dhcps == previous_dhcps + 1);
    for (unsigned i = 12; i <= 40; ++i)
        step(milliseconds + i * 250);
    T(phase == CONNECT_DHCP && connections == count && dhcps == previous_dhcps + 1);
    cmd(NETD_PAUSE, 0);
    T(phase == CONNECT_IDLE && !link && !status.result);
    step(milliseconds + 11000);
    T(status.state == NETD_OFF);
    cmd(NETD_RECONNECT, 0);
    settle(520000);
    T(phase == CONNECT_DHCP);
    step(550000);
    T(status.state == NETD_WAITING && phase == CONNECT_IDLE && !link);
    dhcp_pending = false;
    settle(551000);
    T(status.state == NETD_ONLINE);
    /* A failed replacement preserves the previous durable generation. */
    uint64_t generation = profiles.generation;
    write_fail = true;
    cmd(NETD_AUTO_OFF, 0);
    T(status.result == -LUME_EIO && profiles.automatic && profiles.generation == generation);
    write_fail = false;
    T(load() && profiles.generation == generation && profiles.automatic);
    /* Both existing slots invalid must never re-import the old credentials. */
    files[0].data[0] ^= 1;
    files[1].data[0] ^= 1;
    T(!load());
    files[0].exists = files[1].exists = false;
    T(load() && profiles.count == 1);
    cmd(NETD_FORGET, "current");
    T(!status.result && !profiles.count);
    T(load() && !profiles.count); /* empty configuration stays empty */
    cmd(NETD_IMPORT, "current");
    T(!status.result && profiles.count == 1);
    for (unsigned i = 3; i < 8; ++i)
        T(!descriptors[i].used);
    T(drops >= 4);
    /* Ethernet acquires a lease while Wi-Fi is paused and has no profiles. */
    count = connections;
    paused = true;
    net_wipe(&profiles, sizeof(profiles));
    ethernet_present = ethernet_admin = true;
    ethernet_step(600000);
    T(ethernet_dhcps == 0);
    ethernet_link = true;
    ethernet_pending = true;
    ethernet_step(600001);
    T(ethernet_dhcps == 1 && ethernet[0].pending && !ethernet_address);
    for (unsigned i = 1; i < 100; ++i)
        ethernet_step(600001 + i * 250);
    T(ethernet_dhcps == 1 && connections == count);
    ethernet_address = true;
    ethernet_step(625001);
    T(ethernet[0].online && !ethernet[0].pending && !ethernet[0].attempts);
    ethernet_step(625251);
    T(ethernet_dhcps == 1);
    ethernet_admin = ethernet_address = false;
    ethernet_step(626000);
    T(!ethernet[0].online && !ethernet[0].pending && ethernet_dhcps == 1);
    ethernet_admin = true;
    ethernet_step(626001);
    T(ethernet_dhcps == 2 && ethernet[0].pending);
    ethernet_step(656001);
    T(!ethernet[0].pending && ethernet[0].retry_at == 657001);
    ethernet_step(657000);
    T(ethernet_dhcps == 2);
    ethernet_fail = true;
    ethernet_step(657001);
    T(ethernet_dhcps == 3 && ethernet[0].retry_at == 659001);
    ethernet_link = false;
    ethernet_step(657002);
    T(!ethernet[0].attempts && !ethernet[0].retry_at);
    ethernet_link = true;
    ethernet_pending = ethernet_fail = false;
    ethernet_step(657003);
    ethernet_step(657253);
    T(ethernet_dhcps == 4 && ethernet[0].online && ethernet_address);
    T(load());
    unsigned previous_profiles = profiles.count;
    cmd(NETD_IMPORT_FILE, "new");
    T(status.result == -LUME_EIO && profiles.count == previous_profiles);
    files[3] = files[2];
    files[3].data[8] = 'N';
    cmd(NETD_IMPORT_FILE, "new");
    T(!status.result && profiles.count == previous_profiles + 1 && find("new") >= 0);
    T(!files[3].exists);
    T(profiles.profile[find("new")].ssid[0] == 'N');
    T(files[2].data[8] == 'I');
    T(load() && profiles.count == previous_profiles + 1 && find("new") >= 0);
    T(profiles.profile[find("new")].ssid[0] == 'N');
    cmd(NETD_IMPORT_FILE, "../bad");
    T(status.result == -LUME_EINVAL && profiles.count == previous_profiles + 1);
    test_loop = true;
    ethernet_present = false;
    profiles.generation = 0;
    status.acknowledged = 0;
    pending_commands = 11;
    T(!manager_step(false) && pending_commands == 3 && receives == 8 && acknowledgements == 8 &&
      sleeps == 1 && !event_waits);
    T(!manager_step(false) && !pending_commands && receives == 12 && acknowledgements == 11 &&
      sleeps == 1 && event_waits == 1);
    old_wait = true;
    T(!manager_step(false) && sleeps == 2 && event_waits == 2);

    test_clock = true;
    memset(&boot_clock, 0, sizeof(boot_clock));
    clock_step(10, false);
    T(!time_connects && !time_sets);
    clock_step(20, true);
    T(time_connects == 1 && time_sends == 1 && boot_clock.handle == 19);
    time_reply = wrong_nonce = true;
    clock_step(30, true);
    T(!time_sets && !time_closes && boot_clock.handle == 19);
    clock_step(5020, true);
    T(time_closes == 1 && !boot_clock.handle && boot_clock.retry_at == 6020);
    clock_step(6019, true);
    T(time_connects == 1);
    clock_step(6020, true);
    T(time_connects == 2 && time_sends == 2);
    time_reply = true;
    wrong_nonce = false;
    clock_step(6030, true);
    T(time_sets == 1 && time_closes == 2 && utc_valid && boot_clock.settled);
    clock_step(9000, true);
    T(time_connects == 2 && time_sets == 1);
    memset(&boot_clock, 0, sizeof(boot_clock));
    clock_step(10000, true);
    T(boot_clock.settled && time_connects == 2 && time_sets == 1);
    memset(&boot_clock, 0, sizeof(boot_clock));
    utc_valid = false;
    clock_step(11000, true);
    utc_valid = true;
    clock_step(11001, true);
    T(boot_clock.settled && time_connects == 3 && time_closes == 3 && time_sets == 1);
    printf("PASS_NET_USERSPACE_MANAGER checks=%u\n", checks);
    return 0;
}
