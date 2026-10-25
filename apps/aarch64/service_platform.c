#include "service_platform.h"
#include "lume/abi.h"

static int64_t native(unsigned number, uint64_t a, uint64_t b, uint64_t c)
{
    register uint64_t x8 __asm__("x8") = number;
    register uint64_t x0 __asm__("x0") = a, x1 __asm__("x1") = b, x2 __asm__("x2") = c;
    __asm__ volatile("svc #0x4c55" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2) : "memory", "cc");
    return (int64_t)x0;
}

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
