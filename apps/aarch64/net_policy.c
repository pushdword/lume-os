#include "net_policy.h"

void net_wipe(void *p, size_t n)
{
    volatile uint8_t *b = p;
    while (n--)
        *b++ = 0;
}

static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void put32(uint8_t *p, uint32_t v)
{
    for (unsigned i = 0; i < 4; ++i)
        p[i] = (uint8_t)(v >> (8 * i));
}

static uint32_t crc(const uint8_t *p, size_t n)
{
    uint32_t c = ~0u;
    while (n--)
    {
        c ^= *p++;
        for (unsigned i = 0; i < 8; ++i)
            c = (c >> 1) ^ (0xedb88320u & (0u - (c & 1)));
    }
    return ~c;
}

bool net_name(const char *s)
{
    unsigned n = 0;
    for (; n < 16 && s[n]; ++n)
        if (!((s[n] >= 'a' && s[n] <= 'z') || (s[n] >= 'A' && s[n] <= 'Z') ||
              (s[n] >= '0' && s[n] <= '9') || s[n] == '_' || s[n] == '-'))
            return false;
    return n > 0 && n < 16;
}

static bool same(const char *a, const char *b)
{
    for (unsigned i = 0; i < 16; ++i)
    {
        if (a[i] != b[i])
            return false;
        if (!a[i])
            return true;
    }
    return false;
}

bool net_profiles_valid(const struct net_profiles *p)
{
    if (!p->generation || p->automatic > 1 || p->count > NET_PROFILES)
        return false;
    for (unsigned i = 0; i < p->count; ++i)
    {
        const struct net_profile *a = p->profile + i;
        if (!net_name(a->name) || a->priority > 255 || !a->ssid_bytes || a->ssid_bytes > 32 ||
            a->key_bytes < 8 || a->key_bytes > 63)
            return false;
        for (unsigned k = 0; k < a->key_bytes; ++k)
            if (a->key[k] < 32 || a->key[k] > 126)
                return false;
        for (unsigned j = 0; j < i; ++j)
            if (same(a->name, p->profile[j].name))
                return false;
    }
    return true;
}

bool net_encode(const struct net_profiles *p, uint8_t out[NET_STORE_BYTES])
{
    net_wipe(out, NET_STORE_BYTES);
    if (!net_profiles_valid(p))
        return false;
    out[0] = 'L';
    out[1] = 'N';
    out[2] = 'P';
    out[3] = '1';
    put32(out + 4, 1);
    put32(out + 8, (uint32_t)p->generation);
    put32(out + 12, (uint32_t)(p->generation >> 32));
    put32(out + 16, p->automatic);
    put32(out + 20, p->count);
    for (unsigned i = 0; i < p->count; ++i)
    {
        const struct net_profile *a = p->profile + i;
        uint8_t *b = out + 28 + i * 128;
        for (unsigned j = 0; j < 16 && a->name[j]; ++j)
            b[j] = (uint8_t)a->name[j];
        put32(b + 16, a->priority);
        b[20] = a->ssid_bytes;
        b[21] = a->key_bytes;
        for (unsigned j = 0; j < a->ssid_bytes; ++j)
            b[24 + j] = a->ssid[j];
        for (unsigned j = 0; j < a->key_bytes; ++j)
            b[56 + j] = a->key[j];
    }
    put32(out + 540, crc(out, 540));
    return true;
}

bool net_decode(struct net_profiles *p, const uint8_t *b, size_t n)
{
    net_wipe(p, sizeof(*p));
    if (n != NET_STORE_BYTES || b[0] != 'L' || b[1] != 'N' || b[2] != 'P' || b[3] != '1' ||
        get32(b + 4) != 1 || get32(b + 540) != crc(b, 540) || get32(b + 24))
        return false;
    struct net_profiles t = {0};
    t.generation = get32(b + 8) | (uint64_t)get32(b + 12) << 32;
    t.automatic = get32(b + 16);
    t.count = get32(b + 20);
    if (t.count > 4)
        return false;
    for (unsigned i = 0; i < t.count; ++i)
    {
        struct net_profile *a = t.profile + i;
        const uint8_t *s = b + 28 + i * 128;
        for (unsigned j = 0; j < 16; ++j)
            a->name[j] = (char)s[j];
        a->priority = get32(s + 16);
        a->ssid_bytes = s[20];
        a->key_bytes = s[21];
        for (unsigned j = 0; j < 32; ++j)
            a->ssid[j] = s[24 + j];
        for (unsigned j = 0; j < 64; ++j)
            a->key[j] = s[56 + j];
    }
    bool ok = net_profiles_valid(&t);
    uint8_t canonical[NET_STORE_BYTES];
    if (ok)
    {
        (void)net_encode(&t, canonical);
        for (unsigned i = 0; i < NET_STORE_BYTES; ++i)
            if (canonical[i] != b[i])
                ok = false;
    }
    if (ok)
        *p = t;
    net_wipe(&t, sizeof(t));
    net_wipe(canonical, sizeof(canonical));
    return ok;
}

bool net_legacy(struct net_profile *p, const uint8_t *b, size_t n, const char *name)
{
    net_wipe(p, sizeof(*p));
    if (n != 104 || !net_name(name) || b[0] != 'L' || b[1] != 'W' || b[2] != 'N' || b[3] != '1' ||
        !b[4] || b[4] > 32 || b[5] < 8 || b[5] > 63 || b[6] || b[7])
        return false;
    for (unsigned i = 0; name[i]; ++i)
        p->name[i] = name[i];
    p->priority = 128;
    p->ssid_bytes = b[4];
    p->key_bytes = b[5];
    for (unsigned i = 0; i < b[4]; ++i)
        p->ssid[i] = b[8 + i];
    for (unsigned i = 0; i < b[5]; ++i)
    {
        if (b[40 + i] < 32 || b[40 + i] > 126)
        {
            net_wipe(p, sizeof(*p));
            return false;
        }
        p->key[i] = b[40 + i];
    }
    return true;
}

int net_preferred(const struct net_profiles *p, unsigned rotation)
{
    if (!p->count)
        return -1;
    unsigned order[4];
    for (unsigned i = 0; i < p->count; ++i)
        order[i] = i;
    for (unsigned i = 0; i < p->count; ++i)
        for (unsigned j = i + 1; j < p->count; ++j)
            if (p->profile[order[j]].priority > p->profile[order[i]].priority)
            {
                unsigned t = order[i];
                order[i] = order[j];
                order[j] = t;
            }
    return (int)order[rotation % p->count];
}

uint64_t net_retry(uint64_t now, unsigned failures)
{
    unsigned shift = failures > 6 ? 6 : failures;
    uint64_t delay = 1000u << shift;
    if (delay > 60000)
        delay = 60000;
    return now > UINT64_MAX - delay ? UINT64_MAX : now + delay;
}
