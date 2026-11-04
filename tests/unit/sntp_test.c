/* SPDX-License-Identifier: BSD-3-Clause */
#include "../../apps/clock/sntp.h"
#include <assert.h>
#include <stdio.h>

static void word(unsigned char *output, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i)
        output[i] = (unsigned char)(value >> (24 - i * 8));
}

int main(void)
{
    unsigned char packet[48] = {0x24, 2}, nonce[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    memcpy(packet + 24, nonce, 8);
    word(packet + 40, 1793628000u + 2208988800u);
    word(packet + 44, 0x80000000);
    uint64_t seconds;
    uint32_t nanos;
    assert(sntp_reply(packet, 48, nonce, &seconds, &nanos));
    assert(seconds == 1793628000 && nanos == 500000000);
    assert(!sntp_reply(packet, 47, nonce, &seconds, &nanos));
    packet[24] ^= 1;
    assert(!sntp_reply(packet, 48, nonce, &seconds, &nanos));
    packet[24] ^= 1;
    packet[0] |= 0xc0;
    assert(!sntp_reply(packet, 48, nonce, &seconds, &nanos));
    packet[0] = 0x23;
    assert(!sntp_reply(packet, 48, nonce, &seconds, &nanos));
    packet[0] = 0x24;
    packet[1] = 0;
    assert(!sntp_reply(packet, 48, nonce, &seconds, &nanos));
    packet[1] = 16;
    assert(!sntp_reply(packet, 48, nonce, &seconds, &nanos));
    packet[1] = 2;
    word(packet + 40, (uint32_t)(UINT64_C(2208988800) + 2208988800));
    assert(sntp_reply(packet, 48, nonce, &seconds, &nanos));
    assert(seconds == 2208988800);
    word(packet + 40, 2208988801u);
    assert(!sntp_reply(packet, 48, nonce, &seconds, &nanos));
    puts("SNTP_PACKET_PASS");
}
