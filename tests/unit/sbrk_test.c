#include "api.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static int32_t current_break, query_error, update_error;
static unsigned queries, updates, update_bias;

void *sbrk(ptrdiff_t increment);

int32_t lume_syscall(uint32_t number, uint32_t end, uint32_t b, uint32_t c)
{
    assert(number == 35 && !b && !c);
    if (!end)
    {
        ++queries;
        return query_error ? query_error : current_break;
    }
    ++updates;
    if (update_error)
        return update_error;
    current_break = (int32_t)(end + update_bias);
    return current_break;
}

int lume_errno_from_syscall(unsigned error)
{
    assert(error == 12 || error == 5);
    return error == 12 ? ENOMEM : EIO;
}

static void reset(void)
{
    current_break = 0x48002000;
    query_error = update_error = 0;
    queries = updates = update_bias = 0;
    errno = 0;
}

int main(void)
{
    reset();
    errno = EDOM;
    assert(sbrk(0) == (void *)(uintptr_t)0x48002000);
    assert(queries == 1 && !updates && errno == EDOM);

    reset();
    query_error = -12;
    assert(sbrk(0) == (void *)-1 && errno == ENOMEM);
    assert(queries == 1 && !updates);

    reset();
    assert(sbrk(64) == (void *)(uintptr_t)0x48002000);
    assert(current_break == 0x48002040 && queries == 1 && updates == 1);

    reset();
    assert(sbrk(-65) == (void *)(uintptr_t)0x48002000);
    assert(current_break == 0x48001fbf && queries == 1 && updates == 1);

    reset();
    assert(sbrk(-((ptrdiff_t)current_break + 1)) == (void *)-1 && errno == ENOMEM);
    assert(queries == 1 && !updates);

    reset();
    assert(sbrk(PTRDIFF_MIN) == (void *)-1 && errno == ENOMEM);
    assert(queries == 1 && !updates);

    reset();
    update_error = -5;
    assert(sbrk(1) == (void *)-1 && errno == EIO);
    assert(current_break == 0x48002000 && queries == 1 && updates == 1);

    reset();
    update_bias = 1;
    assert(sbrk(1) == (void *)-1 && errno == ENOMEM);
    assert(queries == 1 && updates == 1);

    puts("PASS_SBRK query/growth/shrink/underflow/errors/returned break");
    return 0;
}
