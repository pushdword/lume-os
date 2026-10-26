#include "lume/abi.h"
#include "lume/thread.h"
#include "lume/syscall_native.h"
#include "errno_map.h"
#include <stddef.h>
#include <stdint.h>

int64_t lume_thread_call(unsigned operation, struct lume_thread_request *request)
{
    int64_t result = lume_native_call(LUME_SYS_THREAD, operation, (uintptr_t)request,
                                      request ? sizeof(*request) : 0);
    return result < 0 ? -(int64_t)lume_errno_from_syscall((unsigned)-result) : result;
}
