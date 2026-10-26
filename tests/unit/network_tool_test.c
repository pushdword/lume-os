#define LUME_USER_HOST_TEST 1
#include "../../apps/net/network_tool.h"
#include <assert.h>

static unsigned sleeps, duration[4];

int64_t call(uint64_t number, uint64_t a, uint64_t b, uint64_t c)
{
    assert(number == LUME_SYS_SLEEP && !b && !c && a && a <= 60000);
    assert(sleeps < 4);
    duration[sleeps++] = (unsigned)a;
    return 0;
}

int main(void)
{
    unsigned value;
    unsigned char address[4];
    assert(tool_unsigned("65535", 65535, &value) && value == 65535);
    assert(!tool_unsigned("65536", 65535, &value));
    assert(!tool_unsigned("42949672960", 65535, &value));
    assert(!tool_unsigned("", 65535, &value));
    assert(tool_address("10.0.2.2", address) && address[0] == 10 && address[3] == 2);
    assert(!tool_address("256.0.2.2", address));
    assert(!tool_address("10.0.2.2.extra", address));
    assert(!tool_address("10.0.2", address));
    tool_sleep(0);
    assert(!sleeps);
    tool_sleep(120001);
    assert(sleeps == 3 && duration[0] == 60000 && duration[1] == 60000 && duration[2] == 1);
    return 0;
}
