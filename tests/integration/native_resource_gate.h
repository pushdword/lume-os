#ifndef LUME_NATIVE_RESOURCE_GATE_H
#define LUME_NATIVE_RESOURCE_GATE_H
#include <stdbool.h>
#include <stdint.h>

struct resource_limits
{
    uint64_t query_ms, read_ms, io_ms, gap_ms;
};

struct resource_measurements
{
    int64_t net_result, query_result;
    uint64_t bytes, query_ms, read_ms, io_ms, gap_ms, samples;
    bool display_overlap, storage_overlap;
    int error;
};

static bool positive_budget(const char *text, uint64_t *value)
{
    uint64_t result = 0;
    if (!text || !*text)
        return false;
    for (const char *at = text; *at; ++at)
    {
        if (*at < '0' || *at > '9' || result > (UINT64_MAX - (unsigned)(*at - '0')) / 10)
            return false;
        result = result * 10 + (unsigned)(*at - '0');
    }
    *value = result;
    return result != 0;
}

static bool resource_passed(const struct resource_measurements *measured,
                            const struct resource_limits *limits)
{
    return measured->net_result >= 0 && measured->query_result >= 0 && !measured->error &&
           measured->samples && measured->bytes && measured->display_overlap &&
           measured->storage_overlap && measured->query_ms <= limits->query_ms &&
           measured->read_ms <= limits->read_ms && measured->io_ms <= limits->io_ms &&
           measured->gap_ms <= limits->gap_ms;
}
#endif
