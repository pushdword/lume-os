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

static struct
{
    bool exists;
    unsigned size;
    uint8_t data[NET_STORE_BYTES];
} files[3];

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
    if (number == LUME_SYS_OPEN)
    {
        int index = -1;
        for (unsigned i = 0; i < 2; ++i)
            if (!strcmp((const char *)(uintptr_t)a, stores[i]))
                index = (int)i;
        if (!strcmp((const char *)(uintptr_t)a, "/data/system/wifi.cfg"))
            index = 2;
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
        return a == LUME_SERVICE_RECEIVE ? -LUME_EAGAIN : 0;
    if (number == LUME_SYS_SLEEP)
    {
        milliseconds += a;
        return 0;
    }
    if (number == LUME_SYS_NETCTL)
    {
        struct lume_net_control *r = (void *)(uintptr_t)b;
        switch (a)
        {
        case LUME_NET_ENUM:
            r->count = 1;
            r->data.interfaces[0] = (struct lume_netif_info){.ifindex = 1, .kind = LUME_IF_WIFI};
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
                                                       : link ? LUME_WIFI_CONNECTED : LUME_WIFI_FAILED;
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
    printf("PASS_NET_USERSPACE_MANAGER checks=%u\n", checks);
    return 0;
}
