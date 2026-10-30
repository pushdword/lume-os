#ifndef LUME_NATIVE_POSIX_H
#define LUME_NATIVE_POSIX_H
#include "lume/syscall_native.h"
#include "lume/abi.h"
#include "errno_map.h"
#include <errno.h>
#define native_call lume_native_call
static inline int native_result(int64_t result)
{
    if (result >= 0)
        return (int)result;
    errno = lume_errno_from_syscall((unsigned)-result);
    return -1;
}
static inline int unsupported(void)
{
    errno = ENOSYS;
    return -1;
}
#endif
