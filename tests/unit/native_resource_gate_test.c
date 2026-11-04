#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../integration/native_resource_gate.h"

int main(void)
{
    uint64_t budget;
    assert(positive_budget("1", &budget) && budget == 1);
    assert(positive_budget("18446744073709551615", &budget) && budget == UINT64_MAX);
    const char *invalid[] = {"", "0", "-1", "+1", "1ms", " 1", "18446744073709551616"};
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        assert(!positive_budget(invalid[i], &budget));
    const struct resource_limits limits = {.query_ms = 2, .read_ms = 8, .io_ms = 3, .gap_ms = 4};
    const struct resource_measurements valid = {.bytes = 65536,
                                                .query_ms = 2,
                                                .read_ms = 8,
                                                .io_ms = 3,
                                                .gap_ms = 4,
                                                .samples = 2,
                                                .display_overlap = true,
                                                .storage_overlap = true};
    assert(resource_passed(&valid, &limits));
    struct resource_measurements measured = valid;
    measured.display_overlap = false;
    assert(!resource_passed(&measured, &limits));
    measured = valid;
    measured.storage_overlap = false;
    assert(!resource_passed(&measured, &limits));
    measured = valid;
    measured.query_ms = limits.query_ms + 1;
    assert(!resource_passed(&measured, &limits));
    measured = valid;
    measured.read_ms = limits.read_ms + 1;
    assert(!resource_passed(&measured, &limits));
    measured = valid;
    measured.io_ms = limits.io_ms + 1;
    assert(!resource_passed(&measured, &limits));
    measured = valid;
    measured.gap_ms = limits.gap_ms + 1;
    assert(!resource_passed(&measured, &limits));
    measured = valid;
    measured.net_result = -1;
    assert(!resource_passed(&measured, &limits));
    measured = valid;
    measured.query_result = -1;
    assert(!resource_passed(&measured, &limits));
    measured = valid;
    measured.error = 1;
    assert(!resource_passed(&measured, &limits));
    measured = valid;
    measured.samples = 0;
    assert(!resource_passed(&measured, &limits));
    measured = valid;
    measured.bytes = 0;
    assert(!resource_passed(&measured, &limits));
    puts("PASS_RESOURCE_GATE overlap, explicit budgets, boundaries and failure results");
    return 0;
}
