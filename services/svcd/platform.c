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
