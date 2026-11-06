#define _POSIX_C_SOURCE 200809L
#include "pthread.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

struct trace
{
    unsigned events[16], count, destroyed;
    pthread_mutex_t resource;
};

struct action
{
    struct trace *trace;
    unsigned event;
};

static _Thread_local struct trace *current;
static pthread_key_t key;
static pthread_mutex_t gate_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t gate_condition = PTHREAD_COND_INITIALIZER;
static unsigned ready, go;

static void record(void *argument)
{
    const struct action *action = argument;
    assert(action->trace == current);
    assert(current->count < sizeof(current->events) / sizeof(current->events[0]));
    current->events[current->count++] = action->event;
}

static void destroy(void *argument)
{
    struct trace *trace = argument;
    assert(trace == current);
    assert(pthread_mutex_trylock(&trace->resource) == 0);
    assert(pthread_mutex_unlock(&trace->resource) == 0);
    struct action action = {trace, 3 + trace->destroyed++};
    record(&action);
    if (trace->destroyed == 1)
        assert(pthread_setspecific(key, trace) == 0);
}

static void unlock(void *argument)
{
    assert(argument == current);
    assert(pthread_mutex_unlock(&current->resource) == 0);
}

static void reenter(void *argument)
{
    struct action inner = {current, 6};
    record(argument);
    pthread_cleanup_push(record, &inner);
    pthread_cleanup_pop(1);
}

static void exit_from_pop(void *argument)
{
    record(argument);
    pthread_exit((void *)(uintptr_t)77);
}

static void *nested(void *argument)
{
    current = argument;
    struct action outer = {current, 1}, inner = {current, 2};
    pthread_cleanup_push(record, &outer);
    pthread_cleanup_push(record, &inner);
    assert(pthread_mutex_lock(&gate_mutex) == 0);
    ++ready;
    assert(pthread_cond_broadcast(&gate_condition) == 0);
    while (!go)
        assert(pthread_cond_wait(&gate_condition, &gate_mutex) == 0);
    assert(pthread_mutex_unlock(&gate_mutex) == 0);
    pthread_cleanup_pop(1);
    pthread_cleanup_pop(1);
    return (void *)(uintptr_t)71;
}

static void *explicit_exit(void *argument)
{
    current = argument;
    assert(pthread_setspecific(key, current) == 0);
    assert(pthread_mutex_lock(&current->resource) == 0);
    struct action outer = {current, 1}, inner = {current, 2};
    pthread_cleanup_push(unlock, current);
    pthread_cleanup_push(record, &outer);
    pthread_cleanup_push(record, &inner);
    pthread_exit((void *)(uintptr_t)73);
    pthread_cleanup_pop(0);
    pthread_cleanup_pop(0);
    pthread_cleanup_pop(0);
}

static void *pop_exit(void *argument)
{
    current = argument;
    assert(pthread_setspecific(key, current) == 0);
    struct action outer = {current, 1}, inner = {current, 2};
    pthread_cleanup_push(record, &outer);
    pthread_cleanup_push(exit_from_pop, &inner);
    pthread_cleanup_pop(1);
    pthread_cleanup_pop(0);
    return NULL;
}

static void *normal_return(void *argument)
{
    current = argument;
    assert(pthread_setspecific(key, current) == 0);
    struct action ignored = {current, 9};
    pthread_cleanup_push(record, &ignored);
    pthread_cleanup_pop(0);
    return (void *)(uintptr_t)79;
}

static void check_events(const struct trace *trace, const unsigned *events, unsigned count)
{
    assert(trace->count == count);
    for (unsigned i = 0; i < count; ++i)
        assert(trace->events[i] == events[i]);
}

static void run_one(void *(*function)(void *), uintptr_t result, const unsigned *events,
                    unsigned count)
{
    struct trace trace = {0};
    pthread_t thread;
    void *value = NULL;
    assert(pthread_create(&thread, NULL, function, &trace) == 0);
    assert(pthread_join(thread, &value) == 0);
    assert(value == (void *)result);
    check_events(&trace, events, count);
    assert(pthread_mutex_destroy(&trace.resource) == 0);
}

int main(void)
{
    struct trace trace = {0};
    current = &trace;
    struct action action = {&trace, 5};
    unsigned routine_evaluations = 0, argument_evaluations = 0, execute_evaluations = 0;
    errno = EDOM;
    pthread_cleanup_push((++routine_evaluations, reenter), (++argument_evaluations, &action));
    pthread_cleanup_pop(++execute_evaluations);
    assert(routine_evaluations == 1 && argument_evaluations == 1 && execute_evaluations == 1);
    assert(errno == EDOM);
    const unsigned reentrant_events[] = {5, 6};
    check_events(&trace, reentrant_events, 2);
    pthread_cleanup_push(record, &action);
    pthread_cleanup_pop(0);
    assert(errno == EDOM);
    check_events(&trace, reentrant_events, 2);

    struct trace concurrent[2] = {0};
    pthread_t threads[2];
    for (unsigned i = 0; i < 2; ++i)
        assert(pthread_create(&threads[i], NULL, nested, &concurrent[i]) == 0);
    assert(pthread_mutex_lock(&gate_mutex) == 0);
    while (ready < 2)
        assert(pthread_cond_wait(&gate_condition, &gate_mutex) == 0);
    go = 1;
    assert(pthread_cond_broadcast(&gate_condition) == 0);
    assert(pthread_mutex_unlock(&gate_mutex) == 0);
    const unsigned nested_events[] = {2, 1};
    for (unsigned i = 0; i < 2; ++i)
    {
        void *value = NULL;
        assert(pthread_join(threads[i], &value) == 0);
        assert(value == (void *)(uintptr_t)71);
        check_events(&concurrent[i], nested_events, 2);
    }
    assert(pthread_key_create(&key, destroy) == 0);
    const unsigned exit_events[] = {2, 1, 3, 4};
    run_one(explicit_exit, 73, exit_events, 4);
    run_one(pop_exit, 77, exit_events, 4);
    const unsigned normal_events[] = {3, 4};
    run_one(normal_return, 79, normal_events, 2);
    assert(pthread_key_delete(key) == 0);
    assert(pthread_mutex_destroy(&gate_mutex) == 0);
    assert(pthread_cond_destroy(&gate_condition) == 0);
    puts("pthread cleanup PASS: nesting, TLS, pop, exit, reentrancy and TSD order");
    return 0;
}
