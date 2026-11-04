#include "service_platform.h"
#include "lume/abi.h"

#include "lume/syscall_native.h"
#define native lume_native_call

int service_call(unsigned operation, struct lume_supervision_request *request)
{
    return (int)native(LUME_SYS_SUPERVISE, operation, (uintptr_t)request, sizeof(*request));
}

uint64_t service_now(void)
{
    return (uint64_t)native(LUME_SYS_CLOCK, 0, 0, 0);
}

void service_sleep(unsigned milliseconds)
{
    (void)native(LUME_SYS_SLEEP, milliseconds, 0, 0);
}

int service_wait(uint64_t sequence, uint64_t timeout)
{
    struct lume_supervision_wait request = {.version = LUME_SUPERVISION_VERSION,
                                            .bytes = sizeof(request),
                                            .sequence = sequence,
                                            .timeout_ms = timeout};
    return (int)native(LUME_SYS_SUPERVISE, LUME_SV_WAIT, (uintptr_t)&request, sizeof(request));
}
