#define _WIN32_WINNT 0x0602
#include "../fixtures/pthread_host_namespace.h"
#include "pthread.h"
#include "lume/abi.h"
#include <assert.h>
#include <errno.h>
#include <process.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

pid_t lume_test_fork(void);

static CRITICAL_SECTION registry;
static _Thread_local unsigned lock_depth;
static unsigned events[64], event_count, allocations;
static int fail_allocation, added_prepare, added_parent, added_child;
static int64_t fork_result;
static unsigned concurrent_prepare, concurrent_parent;
static bool key_registry_locked;

void *atfork_test_malloc(size_t bytes)
{
    assert(lock_depth == 0);
    errno = EDOM;
    if (fail_allocation)
        return NULL;
    void *result = malloc(bytes);
    assert(result);
    __atomic_add_fetch(&allocations, 1, __ATOMIC_RELAXED);
    return result;
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    (void)mutex;
    EnterCriticalSection(&registry);
    assert(lock_depth++ == 0);
    errno = EAGAIN;
    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    (void)mutex;
    assert(lock_depth-- == 1);
    LeaveCriticalSection(&registry);
    errno = EAGAIN;
    return 0;
}

static void record(unsigned event)
{
    assert(lock_depth == 0 && event_count < sizeof(events) / sizeof(events[0]));
    assert(event == 99 ? key_registry_locked : !key_registry_locked);
    events[event_count++] = event;
}

static void prepare_c(void)
{
    record(30);
}

static void parent_c(void)
{
    record(31);
}

static void child_c(void)
{
    record(32);
}

static void prepare_d(void)
{
    record(40);
}

static void parent_d(void)
{
    record(41);
}

static void child_d(void)
{
    record(42);
}

static void prepare_e(void)
{
    record(50);
}

static void parent_e(void)
{
    record(51);
}

static void child_e(void)
{
    record(52);
}

static void prepare_a(void)
{
    record(10);
}

static void parent_a(void)
{
    record(11);
    if (!added_parent++)
        assert(pthread_atfork(prepare_d, parent_d, child_d) == 0);
}

static void child_a(void)
{
    record(12);
}

static void prepare_b(void)
{
    record(20);
    if (!added_prepare++)
        assert(pthread_atfork(prepare_c, parent_c, child_c) == 0);
}

static void parent_b(void)
{
    record(21);
    errno = ERANGE;
}

static void child_b(void)
{
    record(22);
    if (!added_child++)
        assert(pthread_atfork(prepare_e, parent_e, child_e) == 0);
}

int64_t lume_native_call(uint64_t number, uint64_t a, uint64_t b, uint64_t c)
{
    assert(number == LUME_SYS_FORK && a == 0 && b == 0 && c == 0);
    record(99);
    return fork_result;
}

void lume_pthread_fork_prepare(void)
{
    assert(!key_registry_locked);
    key_registry_locked = true;
}

void lume_pthread_fork_parent(void)
{
    assert(key_registry_locked);
    key_registry_locked = false;
}

void lume_pthread_fork_child(void)
{
    assert(key_registry_locked);
    key_registry_locked = false;
    record(98);
}

static void check(int64_t result, int returned, int expected_errno, const unsigned *expected,
                  unsigned count)
{
    event_count = 0;
    fork_result = result;
    errno = E2BIG;
    assert(lume_test_fork() == returned);
    if (expected_errno)
        assert(errno == expected_errno);
    assert(event_count == count);
    for (unsigned i = 0; i < count; ++i)
        assert(events[i] == expected[i]);
}

static void count_prepare(void)
{
    assert(lock_depth == 0);
    ++concurrent_prepare;
}

static void count_parent(void)
{
    assert(lock_depth == 0);
    ++concurrent_parent;
}

static unsigned __stdcall register_many(void *argument)
{
    (void)argument;
    for (unsigned i = 0; i < 64; ++i)
    {
        errno = E2BIG;
        assert(pthread_atfork(count_prepare, count_parent, NULL) == 0);
        assert(errno == E2BIG);
    }
    return 0;
}

int main(void)
{
    InitializeCriticalSection(&registry);
    errno = E2BIG;
    assert(pthread_atfork(prepare_a, parent_a, child_a) == 0 && errno == E2BIG);
    assert(pthread_atfork(NULL, NULL, NULL) == 0 && errno == E2BIG);
    fail_allocation = 1;
    assert(pthread_atfork(prepare_e, parent_e, child_e) == ENOMEM && errno == E2BIG);
    fail_allocation = 0;
    assert(pthread_atfork(prepare_b, parent_b, child_b) == 0 && errno == E2BIG);
    const unsigned first[] = {20, 10, 99, 11, 21};
    check(7, 7, 0, first, sizeof(first) / sizeof(first[0]));
    const unsigned later_parent[] = {40, 30, 20, 10, 99, 11, 21, 31, 41};
    check(-LUME_EBUSY, -1, EBUSY, later_parent, sizeof(later_parent) / sizeof(later_parent[0]));
    check(INT64_C(0x100000009), 9, 0, later_parent, sizeof(later_parent) / sizeof(later_parent[0]));
    const unsigned child[] = {40, 30, 20, 10, 99, 98, 12, 22, 32, 42};
    check(0, 0, 0, child, sizeof(child) / sizeof(child[0]));
    const unsigned last[] = {50, 40, 30, 20, 10, 99, 11, 21, 31, 41, 51};
    check(-LUME_ENOMEM, -1, ENOMEM, last, sizeof(last) / sizeof(last[0]));
    assert(allocations == 6);
    HANDLE threads[8];
    for (unsigned i = 0; i < 8; ++i)
    {
        threads[i] = (HANDLE)_beginthreadex(NULL, 0, register_many, NULL, 0, NULL);
        assert(threads[i]);
    }
    assert(WaitForMultipleObjects(8, threads, TRUE, 30000) == WAIT_OBJECT_0);
    for (unsigned i = 0; i < 8; ++i)
        assert(CloseHandle(threads[i]));
    check(8, 8, 0, last, sizeof(last) / sizeof(last[0]));
    assert(concurrent_prepare == 512 && concurrent_parent == 512 && allocations == 518);
    DeleteCriticalSection(&registry);
    puts("atfork PASS: ordering, snapshots, callback registration, errno and concurrent append");
    return 0;
}

#define malloc atfork_test_malloc
#include "../../libc/posix/atfork.c"
#undef malloc

static int native_result(int64_t result)
{
    if (result >= 0)
        return (int)result;
    assert(result == -LUME_EBUSY || result == -LUME_ENOMEM);
    errno = result == -LUME_EBUSY ? EBUSY : ENOMEM;
    return -1;
}

#define native_call lume_native_call
#define fork lume_test_fork
#include "../../libc/posix/process_fork.h"
#undef fork
#undef native_call
