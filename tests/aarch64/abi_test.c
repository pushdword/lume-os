#define lume_stat lume_native_stat
#define LUME_SEEK_SET LUME_NATIVE_SEEK_SET
#define LUME_SEEK_CUR LUME_NATIVE_SEEK_CUR
#define LUME_SEEK_END LUME_NATIVE_SEEK_END
#include "lume/abi.h"
#undef lume_stat
#undef LUME_SEEK_SET
#undef LUME_SEEK_CUR
#undef LUME_SEEK_END
#undef LUME_ARG_BYTES
#include "api.h"
#include <assert.h>
#include <stdio.h>

static uint64_t size = 12;
static int64_t position;
static uint32_t kind = LUME_NODE_FILE;
static int stat_error;
static int64_t wait_result = -11;
static unsigned wait_calls;
static unsigned wait_block;

int64_t lume_abi_host_call(unsigned number, uint64_t a, uint64_t b, uint64_t c)
{
    if (number == LUME_SYS_CPU)
    {
        assert(!b && !c && a <= 2);
        return a == 2 ? 17 : a == 1 ? 3 : 4;
    }
    if (number == LUME_SYS_WAIT)
    {
        assert(!a && b == wait_block && !c);
        ++wait_calls;
        return wait_result;
    }
    assert(a == 3);
    if (number == LUME_SYS_STAT)
    {
        if (stat_error)
            return stat_error;
        *(struct lume_native_stat *)(uintptr_t)b = (struct lume_native_stat){1, size, kind, 0};
        return 0;
    }
    assert(number == LUME_SYS_SEEK);
    if (kind == LUME_NODE_CHAR)
        return -29;
    assert(c <= 2);
    position = (c == 0 ? 0 : c == 1 ? position : (int64_t)size) + (int64_t)b;
    return position;
}

int main(void)
{
    assert(lume_syscall(LUME_INFO, LUME_QUANTUM_MS, 0, 0) == 17);
    assert(lume_syscall(LUME_INFO, LUME_CORES, 0, 0) == 4);
    assert(lume_syscall(LUME_INFO, LUME_CORE, 0, 0) == 3);
    assert(lume_syscall(LUME_SEEK, 3, 0, LUME_SEEK_SET) == 0);
    assert(lume_syscall(LUME_SEEK, 3, 12, LUME_SEEK_SET) == 12);
    assert(lume_syscall(LUME_SEEK, 3, 1, LUME_SEEK_CUR) == -22 && position == 12);
    assert(lume_syscall(LUME_SEEK, 3, (uint32_t)-1, LUME_SEEK_END) == 11);
    assert(lume_syscall(LUME_SEEK, 3, (uint32_t)-13, LUME_SEEK_END) == -22 && position == 11);
    assert(lume_syscall(LUME_SEEK, 3, 0, 3) == -22);
    position = INT64_MAX;
    assert(lume_syscall(LUME_SEEK, 3, 1, LUME_SEEK_CUR) == -22 && position == INT64_MAX);
    size = INT64_MAX;
    assert(lume_syscall(LUME_SEEK, 3, 1, LUME_SEEK_END) == -22);
    size = UINT32_MAX;
    assert(lume_syscall(LUME_SEEK, 3, 0, LUME_SEEK_END) == -75);
    kind = LUME_NODE_CHAR;
    assert(lume_syscall(LUME_SEEK, 3, 0, LUME_SEEK_CUR) == -29);
    stat_error = -9;
    assert(lume_syscall(LUME_SEEK, 3, 0, LUME_SEEK_SET) == -9);
    assert(lume_syscall(LUME_WAITPID, UINT32_MAX, 0, 1) == 0 && wait_calls == 1);
    wait_result = (INT64_C(4) << 32) | 7;
    assert(lume_syscall(LUME_WAITPID, UINT32_MAX, 0, 1) == 4 && wait_calls == 2);
    wait_block = 1;
    assert(lume_syscall(LUME_WAITPID, UINT32_MAX, 0, 0) == 4 && wait_calls == 3);
    puts("PASS_COMMON_ABI seek bounds, signed offsets, overflow, access and stream errors");
    return 0;
}
