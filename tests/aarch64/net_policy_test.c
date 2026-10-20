#include "net_policy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define T(x)                                                                                       \
    do                                                                                             \
    {                                                                                              \
        ++checks;                                                                                  \
        if (!(x))                                                                                  \
        {                                                                                          \
            fprintf(stderr, "NET POLICY FAIL %d: %s\n", __LINE__, #x);                             \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

int main(void)
{
    unsigned char legacy[104] = {'L', 'W', 'N', '1', 4, 8};
    memcpy(legacy + 8, "IEEE", 4);
    memcpy(legacy + 40, "password", 8);
    struct net_profiles p = {.generation = 1, .automatic = 1, .count = 1}, q;
    T(net_legacy(&p.profile[0], legacy, sizeof(legacy), "current"));
    uint8_t raw[NET_STORE_BYTES], bad[NET_STORE_BYTES];
    T(net_encode(&p, raw));
    T(net_decode(&q, raw, sizeof(raw)));
    T(q.count == 1 && q.automatic == 1 && q.generation == 1);
    T(!memcmp(q.profile[0].key, "password", 8));
    for (unsigned n = 0; n < sizeof(raw); ++n)
        T(!net_decode(&q, raw, n));
    for (unsigned i = 0; i < sizeof(raw); ++i)
        for (unsigned b = 0; b < 8; ++b)
        {
            memcpy(bad, raw, sizeof(bad));
            bad[i] ^= (uint8_t)(1u << b);
            T(!net_decode(&q, bad, sizeof(bad)));
            T(!q.count && !q.generation);
        }
    T(!net_decode(&q, raw, sizeof(raw) + 1));
    /* Alternate-slot power-loss model: every incomplete replacement is rejected
     * while the exact previous slot remains decodable. */
    p.generation = 2;
    p.automatic = 0;
    T(net_encode(&p, bad));
    for (unsigned cut = 0; cut < sizeof(raw); ++cut)
    {
        uint8_t torn[NET_STORE_BYTES];
        memset(torn, 0xff, sizeof(torn));
        memcpy(torn, bad, cut);
        T(!net_decode(&q, torn, sizeof(torn)));
        T(net_decode(&q, raw, sizeof(raw)) && q.generation == 1 && q.automatic == 1);
    }
    T(net_decode(&q, bad, sizeof(bad)) && q.generation == 2 && !q.automatic);
    p.count = 4;
    for (unsigned i = 1; i < 4; ++i)
    {
        p.profile[i] = p.profile[0];
        p.profile[i].name[0] = (char)('A' + i);
        p.profile[i].priority = i * 32;
    }
    p.profile[0].priority = 255;
    T(net_preferred(&p, 0) == 0);
    T(net_preferred(&p, 1) == 3);
    T(net_preferred(&p, 2) == 2);
    T(net_preferred(&p, 3) == 1);
    T(net_preferred(&p, 4) == 0);
    T(net_encode(&p, raw));
    T(net_decode(&q, raw, sizeof(raw)));
    T(q.count == 4);
    p.profile[1] = p.profile[0];
    T(!net_encode(&p, raw));
    p.count = 0;
    T(net_encode(&p, raw));
    T(net_preferred(&p, 0) == -1);
    T(net_decode(&q, raw, sizeof(raw)) && !q.count);
    p.automatic = 2;
    T(!net_encode(&p, raw));
    p.automatic = 1;
    p.count = 5;
    T(!net_encode(&p, raw));
    p.count = 1;
    p.profile[0].key_bytes = 64;
    T(!net_encode(&p, raw));
    p.profile[0].key_bytes = 8;
    p.profile[0].key[0] = 1;
    T(!net_encode(&p, raw));
    T(net_name("abc-123_X"));
    T(!net_name(""));
    T(!net_name("../bad"));
    T(!net_name("abcdefghijklmnop"));
    T(!net_name("has space"));
    legacy[6] = 1;
    T(!net_legacy(&p.profile[0], legacy, sizeof(legacy), "current"));
    legacy[6] = 0;
    legacy[5] = 64;
    T(!net_legacy(&p.profile[0], legacy, sizeof(legacy), "current"));
    T(net_retry(100, 0) == 1100);
    T(net_retry(100, 1) == 2100);
    T(net_retry(100, 5) == 32100);
    T(net_retry(100, 6) == 60100);
    T(net_retry(100, 999) == 60100);
    T(net_retry(UINT64_MAX - 3, 2) == UINT64_MAX);
    net_wipe(&p, sizeof(p));
    for (unsigned i = 0; i < sizeof(p); ++i)
        T(!((uint8_t *)&p)[i]);
    printf("PASS_NET_USERSPACE_POLICY checks=%u\n", checks);
    return 0;
}
