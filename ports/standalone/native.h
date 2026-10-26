#ifndef LUME_PORT_NATIVE_H
#define LUME_PORT_NATIVE_H
#include <stdint.h>
#include <errno.h>
#include <errno_map.h>
#include <lume/abi.h>
#include <lume/syscall_native.h>

static inline int64_t native_call(unsigned number, uint64_t a, uint64_t b, uint64_t c)
{
    return lume_native_call(number, a, b, c);
}

static inline int native_result(unsigned number, uint64_t a, uint64_t b, uint64_t c)
{
    int64_t result = native_call(number, a, b, c);
    if (result < 0)
    {
        errno = lume_errno_from_syscall((unsigned)-result);
        return -1;
    }
    return (int)result;
}
#endif
