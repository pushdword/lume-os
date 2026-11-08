/* SPDX-License-Identifier: BSD-3-Clause */
#define LUME_PTHREAD_HOST_HARNESS_ONLY 1
#include "pthread_cleanup_test.c"

static pthread_key_t tls_key, tls_reused;
static HANDLE tls_ready, tls_go, tls_done;
static unsigned tls_ready_count, tls_destroyed;

static void tls_destroy(void *value)
{
    assert(value);
    __atomic_add_fetch(&tls_destroyed, 1, __ATOMIC_RELAXED);
}

static void *tls_generation_reader(void *argument)
{
    assert(!pthread_setspecific(tls_key, argument));
    assert(pthread_getspecific(tls_key) == argument);
    if (__atomic_add_fetch(&tls_ready_count, 1, __ATOMIC_RELEASE) == 4)
        assert(SetEvent(tls_ready));
    assert(WaitForSingleObject(tls_go, 10000) == WAIT_OBJECT_0);
    assert(!pthread_getspecific(tls_key));
    assert(!pthread_getspecific(tls_reused));
    assert(!pthread_setspecific(tls_reused, argument));
    assert(pthread_getspecific(tls_reused) == argument);
    return NULL;
}

static void *tls_no_lock_reader(void *argument)
{
    assert(!pthread_setspecific(tls_key, argument));
    assert(SetEvent(tls_ready));
    assert(WaitForSingleObject(tls_go, 10000) == WAIT_OBJECT_0);
    allocation_budget = 0;
    unsigned before = __atomic_load_n(&allocations, __ATOMIC_RELAXED);
    for (unsigned i = 0; i < 16384; ++i)
        assert(pthread_getspecific(tls_key) == argument);
    assert(__atomic_load_n(&allocations, __ATOMIC_RELAXED) == before);
    allocation_budget = -1;
    assert(SetEvent(tls_done));
    return NULL;
}

static void *tls_allocation_reader(void *argument)
{
    allocation_budget = 0;
    assert(pthread_setspecific(tls_key, argument) == ENOMEM);
    assert(!pthread_getspecific(tls_key));
    assert(!pthread_setspecific(tls_key, NULL));
    allocation_budget = -1;
    assert(!pthread_setspecific(tls_key, argument));
    assert(pthread_getspecific(tls_key) == argument);
    return NULL;
}

static _Thread_local unsigned tls_passes;

static void tls_reinsert(void *value)
{
    ++tls_passes;
    __atomic_add_fetch(&tls_destroyed, 1, __ATOMIC_RELAXED);
    if (tls_passes < PTHREAD_DESTRUCTOR_ITERATIONS)
        assert(!pthread_setspecific(tls_key, value));
}

static void *tls_destructor_reader(void *argument)
{
    assert(!pthread_setspecific(tls_key, argument));
    return NULL;
}

#ifndef LUME_TEST_TLS_BENCH_ONLY
static void tls_functional(void)
{
    assert(pthread_key_create(NULL, NULL) == EINVAL);
    assert(pthread_key_delete(0) == EINVAL && !pthread_getspecific(0));
    assert(pthread_setspecific(0, NULL) == EINVAL);
    pthread_t thread;
    int value = 37;
    assert(!pthread_key_create(&tls_key, NULL));
    assert(!pthread_create(&thread, NULL, tls_allocation_reader, &value));
    assert(!pthread_join(thread, NULL));
    assert(!pthread_key_delete(tls_key));

    assert(!pthread_key_create(&tls_key, NULL));
    assert(!pthread_create(&thread, NULL, tls_no_lock_reader, &value));
    assert(WaitForSingleObject(tls_ready, 10000) == WAIT_OBJECT_0);
    assert(!pthread_mutex_lock(&key_lock));
    assert(SetEvent(tls_go));
    assert(WaitForSingleObject(tls_done, 1000) == WAIT_OBJECT_0);
    assert(!pthread_mutex_unlock(&key_lock));
    assert(!pthread_join(thread, NULL));
    assert(!pthread_key_delete(tls_key));
    ResetEvent(tls_ready);
    ResetEvent(tls_go);
    ResetEvent(tls_done);

    tls_ready_count = tls_destroyed = 0;
    assert(!pthread_key_create(&tls_key, tls_destroy));
    pthread_t readers[4];
    for (unsigned i = 0; i < 4; ++i)
        assert(!pthread_create(&readers[i], NULL, tls_generation_reader, &value));
    assert(WaitForSingleObject(tls_ready, 10000) == WAIT_OBJECT_0);
    assert(!pthread_key_delete(tls_key));
    assert(!pthread_key_create(&tls_reused, tls_destroy));
    assert(tls_key != tls_reused && (tls_key & KEY_SLOT_MASK) == (tls_reused & KEY_SLOT_MASK));
    assert(SetEvent(tls_go));
    for (unsigned i = 0; i < 4; ++i)
        assert(!pthread_join(readers[i], NULL));
    assert(tls_destroyed == 4);
    assert(!pthread_key_delete(tls_reused));

    tls_destroyed = 0;
    assert(!pthread_key_create(&tls_key, tls_reinsert));
    assert(!pthread_create(&thread, NULL, tls_destructor_reader, &value));
    assert(!pthread_join(thread, NULL));
    assert(tls_destroyed == PTHREAD_DESTRUCTOR_ITERATIONS);
    assert(!pthread_key_delete(tls_key));

    pthread_key_t exhausted = 0, key;
    for (unsigned i = 0; i < KEY_GENERATION_MAX; ++i)
    {
        assert(!pthread_key_create(&key, NULL));
        unsigned generation = key >> KEY_SLOT_BITS;
        assert(!pthread_key_delete(key));
        if (generation == KEY_GENERATION_MAX)
        {
            exhausted = key;
            break;
        }
    }
    assert(exhausted);
    assert(!pthread_key_create(&key, NULL));
    assert((key & KEY_SLOT_MASK) != (exhausted & KEY_SLOT_MASK));
    assert(!pthread_getspecific(exhausted) && pthread_setspecific(exhausted, &value) == EINVAL);
    assert(!pthread_key_delete(key));

    pthread_key_t filled[KEY_SEGMENT_SIZE + 1];
    unsigned filled_count = 0;
    allocation_budget = 0;
    key = UINT_MAX;
    int error;
    while (filled_count < KEY_SEGMENT_SIZE && !(error = pthread_key_create(&key, NULL)))
        filled[filled_count++] = key;
    assert(filled_count < KEY_SEGMENT_SIZE && error == EAGAIN);
    allocation_budget = -1;
    assert(!pthread_key_create(&key, NULL));
    filled[filled_count++] = key;
    for (unsigned i = 0; i < filled_count; ++i)
        assert(!pthread_key_delete(filled[i]));

    assert(!pthread_key_create(&tls_key, NULL));
    assert(!pthread_setspecific(tls_key, &value));
    lume_pthread_fork_prepare();
    lume_pthread_fork_parent();
    assert(pthread_getspecific(tls_key) == &value);
    lume_pthread_fork_prepare();
    lume_pthread_fork_child();
    assert(pthread_getspecific(tls_key) == &value);
    assert(!pthread_key_delete(tls_key));
    puts(
        "PASS_TLS_FUNCTIONAL generation, no-lock/no-allocation lookup, failures, destructors and fork hooks");
}
#endif

struct tls_bench
{
    unsigned id, count;
    pthread_key_t *keys;
    LONGLONG elapsed;
};

static void *tls_bench_reader(void *argument)
{
    struct tls_bench *bench = argument;
    for (unsigned i = 0; i < bench->count; ++i)
        assert(!pthread_setspecific(bench->keys[i], bench));
    if (__atomic_add_fetch(&tls_ready_count, 1, __ATOMIC_RELEASE) == bench->id)
        assert(SetEvent(tls_ready));
    assert(WaitForSingleObject(tls_go, 10000) == WAIT_OBJECT_0);
    LARGE_INTEGER before, after;
    QueryPerformanceCounter(&before);
    for (unsigned i = 0; i < 16384; ++i)
        assert(pthread_getspecific(bench->keys[(i * 17u) % bench->count]) == bench);
    QueryPerformanceCounter(&after);
    bench->elapsed = after.QuadPart - before.QuadPart;
    return NULL;
}

static void tls_benchmark(unsigned threads, unsigned count)
{
    pthread_key_t bench_keys[512];
    pthread_t tids[8];
    struct tls_bench benches[8];
    assert(count <= 512 && threads <= 8);
    ResetEvent(tls_ready);
    ResetEvent(tls_go);
    tls_ready_count = 0;
    for (unsigned i = 0; i < count; ++i)
        assert(!pthread_key_create(&bench_keys[i], NULL));
    for (unsigned i = 0; i < threads; ++i)
    {
        benches[i] = (struct tls_bench){.id = threads, .count = count, .keys = bench_keys};
        assert(!pthread_create(&tids[i], NULL, tls_bench_reader, &benches[i]));
    }
    assert(WaitForSingleObject(tls_ready, 10000) == WAIT_OBJECT_0);
    assert(SetEvent(tls_go));
    LONGLONG total = 0, maximum = 0;
    for (unsigned i = 0; i < threads; ++i)
    {
        assert(!pthread_join(tids[i], NULL));
        total += benches[i].elapsed;
        if (benches[i].elapsed > maximum)
            maximum = benches[i].elapsed;
    }
    for (unsigned i = 0; i < count; ++i)
        assert(!pthread_key_delete(bench_keys[i]));
#ifdef KEY_SEGMENT_SIZE
    unsigned retained = 0;
    for (unsigned group = 0; group < KEY_SEGMENT_SIZE; ++group)
        if (key_segments[group])
        {
            ++retained;
            for (unsigned i = 0; i < KEY_SEGMENT_SIZE; ++i)
                assert(!__atomic_load_n(&key_segments[group]->entries[i].active, __ATOMIC_ACQUIRE));
        }
    assert(allocations == releases + retained);
#else
    assert(allocations == releases && keys == NULL && all_values == NULL);
#endif
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    printf("TLS_COST threads=%u keys=%u calls=%u mean_ns=%.3f max_thread_ns=%.3f\n", threads, count,
           threads * 16384u, (double)total * 1e9 / frequency.QuadPart / (threads * 16384u),
           (double)maximum * 1e9 / frequency.QuadPart / 16384u);
}

int main(int argc, char **argv)
{
    InitializeCriticalSection(&scheduler);
    tls_ready = CreateEvent(NULL, TRUE, FALSE, NULL);
    tls_go = CreateEvent(NULL, TRUE, FALSE, NULL);
    tls_done = CreateEvent(NULL, TRUE, FALSE, NULL);
    assert(tls_ready && tls_go && tls_done);
    if (argc == 2 && !strcmp(argv[1], "--bench"))
    {
        const unsigned cases[][2] = {{1, 1}, {1, 512}, {4, 32}, {8, 1}, {8, 512}};
        for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
            tls_benchmark(cases[i][0], cases[i][1]);
    }
#ifndef LUME_TEST_TLS_BENCH_ONLY
    else
        tls_functional();
#endif
#ifdef KEY_SEGMENT_SIZE
    printf("TLS_MEMORY key_segment=%zu value_segment=%zu thread_directory=%zu max_live=%u\n",
           sizeof(struct thread_key_segment), sizeof(struct thread_value_segment),
           sizeof(key_values), KEY_SLOT_MASK);
#endif
    assert(CloseHandle(tls_ready) && CloseHandle(tls_go) && CloseHandle(tls_done));
    DeleteCriticalSection(&scheduler);
    return 0;
}
