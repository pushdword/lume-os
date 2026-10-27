#include "lume/app_support.h"
#include "lume/activity.h"
#include "perf.h"
#include <stdint.h>

#define SAMPLES 13u
static struct lume_activity_snapshot activity[SAMPLES];
static struct lume_perf_snapshot perf[SAMPLES];

int main(void)
{
    for (unsigned i = 0; i < SAMPLES; ++i)
    {
        unsigned op = !i                 ? LUME_ACTIVITY_START
                      : i + 1 == SAMPLES ? LUME_ACTIVITY_STOP
                                         : LUME_ACTIVITY_READ;
        if (call(LUME_SYS_TELEMETRY, (uintptr_t)&activity[i], sizeof(activity[i]), op) ||
            call(LUME_SYS_TELEMETRY, (uintptr_t)&perf[i], sizeof(perf[i]), LUME_PERF_VERSION))
            return 1;
        if (i + 1 < SAMPLES && call(LUME_SYS_SLEEP, 1000, 0, 0))
            return 2;
    }
    const uint32_t header[] = {UINT32_C(0x4255534c), 1, SAMPLES, sizeof(activity[0]),
                               sizeof(perf[0])};
    if (write_all(1, header, sizeof(header)) != sizeof(header) ||
        write_all(1, activity, sizeof(activity)) != sizeof(activity) ||
        write_all(1, perf, sizeof(perf)) != sizeof(perf))
        return 3;
    return 0;
}
