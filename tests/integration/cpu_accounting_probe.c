#include "lume/app_support.h"
#include "perf.h"
#include <stdint.h>

#define SAMPLES 13u
static struct lume_perf_snapshot samples[SAMPLES];

int main(void)
{
    for (unsigned i = 0; i < SAMPLES; ++i)
    {
        struct lume_perf_snapshot *sample = &samples[i];
        if (call(LUME_SYS_TELEMETRY, (uintptr_t)sample, sizeof(*sample), LUME_PERF_VERSION) ||
            sample->version != LUME_PERF_VERSION || sample->bytes != sizeof(*sample) ||
            !sample->clock_hz || !sample->core_count || sample->core_count > LUME_PERF_CORES ||
            sample->process_count > LUME_PERF_PROCESSES)
            return 1;
        if (i + 1 < SAMPLES && call(LUME_SYS_SLEEP, 1000, 0, 0))
            return 2;
    }
    const uint32_t header[] = {UINT32_C(0x5550434c), 1, SAMPLES, sizeof(samples[0])};
    if (write_all(1, header, sizeof(header)) != sizeof(header) ||
        write_all(1, samples, sizeof(samples)) != sizeof(samples))
        return 3;
    return 0;
}
