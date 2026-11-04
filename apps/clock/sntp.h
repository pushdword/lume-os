/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_SNTP_H
#define LUME_SNTP_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static inline uint32_t sntp_word(const unsigned char *value)
{
    return (uint32_t)value[0] << 24 | (uint32_t)value[1] << 16 | (uint32_t)value[2] << 8 | value[3];
}

static inline bool sntp_reply(const unsigned char *packet, size_t bytes,
                              const unsigned char nonce[8], uint64_t *seconds,
                              uint32_t *nanoseconds)
{
    if (bytes < 48 || (packet[0] >> 6) == 3 || (packet[0] & 7) != 4 || ((packet[0] >> 3) & 7) < 3 ||
        ((packet[0] >> 3) & 7) > 4 || !packet[1] || packet[1] > 15 || memcmp(packet + 24, nonce, 8))
        return false;
    uint64_t timestamp = sntp_word(packet + 40);
    if (!timestamp && !sntp_word(packet + 44))
        return false;
    /* The supported era spans 2020 through 2099, including the 2036 rollover. */
    if (timestamp < UINT64_C(2208988800))
        timestamp += UINT64_C(1) << 32;
    timestamp -= UINT64_C(2208988800);
    if (timestamp < UINT64_C(1577836800) || timestamp >= UINT64_C(4102444800))
        return false;
    *seconds = timestamp;
    *nanoseconds = (uint32_t)(((uint64_t)sntp_word(packet + 44) * 1000000000) >> 32);
    return true;
}
#endif
