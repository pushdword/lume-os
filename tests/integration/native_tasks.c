/* SPDX-License-Identifier: BSD-3-Clause */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <spawn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "lume/abi.h"
#include "lume/process.h"
#include "lume/syscall_native.h"
#include "lume/thread.h"

extern char **environ;
extern int64_t lume_thread_call(unsigned, struct lume_thread_request *);
static _Thread_local unsigned identity = 73;

struct channels
{
    int ready, control;
};

static void *worker(void *opaque)
{
    const struct channels *channels = opaque;
    unsigned char slot = (unsigned char)pthread_self();
    assert(identity == 73 && slot);
    identity = slot;
    volatile unsigned char stack[8192];
    for (unsigned i = 0; i < sizeof(stack); ++i)
        stack[i] = (unsigned char)(i + slot);
    assert(write(channels->ready, &slot, 1) == 1);
    unsigned char value;
    assert(read(channels->control, &value, 1) == 0);
    for (unsigned i = 0; i < sizeof(stack); ++i)
        assert(stack[i] == (unsigned char)(i + slot));
    assert(identity == slot);
    return opaque;
}

static void join(pthread_t *threads, unsigned count, struct channels *channels)
{
    for (unsigned i = 0; i < count; ++i)
    {
        void *value;
        assert(!pthread_join(threads[i], &value) && value == channels);
    }
}

static void *self_detached(void *opaque)
{
    struct lume_thread_request request = {.tid = pthread_self()};
    assert(lume_thread_call(LUME_THREAD_JOIN, &request) == -EDEADLK);
    assert(!pthread_detach(pthread_self()));
    assert(pthread_detach(pthread_self()) == EINVAL);
    return worker(opaque);
}

static void self_detach_reuse(void)
{
    for (unsigned pass = 0; pass < 32; ++pass)
    {
        int ready[2], control[2];
        assert(!pipe(ready) && !pipe(control));
        struct channels channels = {ready[1], control[0]};
        pthread_t thread;
        assert(!pthread_create(&thread, NULL, self_detached, &channels));
        unsigned char slot;
        assert(read(ready[0], &slot, 1) == 1);
        assert(slot == (thread & 255));
        assert(pthread_join(thread, NULL) == EINVAL);
        assert(!close(control[1]));
        struct lume_task_info task;
        int64_t found = 1;
        for (unsigned wait = 0; wait < 2000 && found > 0; ++wait)
        {
            found = lume_native_call(LUME_SYS_PROCESS, slot - 1, (uintptr_t)&task, sizeof(task));
            if (found > 0)
                usleep(1000);
        }
        assert(found == 0);
        assert(!close(ready[0]) && !close(ready[1]) && !close(control[0]));
    }
    puts("THREAD_SELF_DETACH_REUSE_PASS");
}

int main(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "worker"))
    {
        struct channels channels = {3, 4};
        pthread_t threads[7];
        for (unsigned i = 0; i < 7; ++i)
            assert(!pthread_create(threads + i, NULL, worker, &channels));
        join(threads, 7, &channels);
        assert(identity == 73);
        return 0;
    }
    int ready[2], control[2];
    assert(!pipe(ready) && !pipe(control));
    for (unsigned i = 0; i < 2; ++i)
    {
        assert(!fcntl(ready[i], F_SETFD, FD_CLOEXEC));
        assert(!fcntl(control[i], F_SETFD, FD_CLOEXEC));
    }
    assert(ready[1] != 3 && control[0] > 4);
    posix_spawn_file_actions_t actions;
    assert(!posix_spawn_file_actions_init(&actions));
    assert(!posix_spawn_file_actions_adddup2(&actions, ready[1], 3));
    assert(!posix_spawn_file_actions_adddup2(&actions, control[0], 4));
    pid_t children[7];
    char *arguments[] = {argv[0], "worker", NULL};
    for (unsigned i = 0; i < 7; ++i)
        assert(!posix_spawn(children + i, argv[0], &actions, NULL, arguments, environ));
    assert(!posix_spawn_file_actions_destroy(&actions));
    struct channels channels = {ready[1], control[0]};
    pthread_t threads[4], extra;
    for (unsigned i = 0; i < 4; ++i)
        assert(!pthread_create(threads + i, NULL, worker, &channels));
    unsigned highest = 0;
    for (unsigned i = 0; i < 53; ++i)
    {
        unsigned char slot;
        assert(read(ready[0], &slot, 1) == 1);
        if (slot > highest)
            highest = slot;
    }
    assert(highest == 64);
    assert(pthread_create(&extra, NULL, worker, &channels) == EAGAIN);
    assert(!close(control[1]));
    join(threads, 4, &channels);
    for (unsigned i = 0; i < 7; ++i)
    {
        int status;
        assert(waitpid(children[i], &status, 0) == children[i]);
        assert(WIFEXITED(status) && !WEXITSTATUS(status));
    }
    assert(!pthread_create(&extra, NULL, worker, &channels));
    unsigned char slot;
    assert(read(ready[0], &slot, 1) == 1);
    join(&extra, 1, &channels);
    assert(!close(ready[0]) && !close(ready[1]) && !close(control[0]));
    self_detach_reuse();
    puts("TASKS_64_TLS_STACK_PIPE_EXHAUST_REUSE_PASS");
    return 0;
}
