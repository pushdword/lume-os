/* SPDX-License-Identifier: BSD-3-Clause */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "lume/spawn.h"
#include "lume/abi.h"
#include "lume/syscall_native.h"

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
static int release_worker;

static void *worker(void *opaque)
{
    assert(!pthread_mutex_lock(&lock));
    while (!release_worker)
        assert(!pthread_cond_wait(&condition, &lock));
    assert(!pthread_mutex_unlock(&lock));
    return opaque;
}

static void child(void)
{
    char cwd[128];
    assert(getcwd(cwd, sizeof(cwd)) && !strcmp(cwd, "/data/spawn-native"));
    assert(getsid(0) == getpid() && getpgrp() == getpid());
    struct sigaction action;
    assert(!sigaction(SIGUSR1, NULL, &action) && action.sa_handler == SIG_DFL);
    assert(!sigaction(SIGUSR2, NULL, &action) && action.sa_handler == SIG_IGN);
    sigset_t mask;
    assert(!sigprocmask(SIG_SETMASK, NULL, &mask));
    assert(sigismember(&mask, SIGUSR2) && !sigismember(&mask, SIGUSR1));
    assert(!sigismember(&mask, SIGKILL) && !sigismember(&mask, SIGSTOP));
    assert(fcntl(30, F_GETFD) == -1 && errno == EBADF);
    assert(fcntl(31, F_GETFD) == 0 && fcntl(5, F_GETFD) == 0);
    assert(!strcmp(getenv("SPAWN_TEXT"), "ação"));
    assert(write(5, "SPAWN_FILE_PASS\n", 16) == 16);
    assert(write(31, "S", 1) == 1);
}

int main(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "child"))
    {
        child();
        return 0;
    }
    char cwd[128], after[128];
    assert(getcwd(cwd, sizeof(cwd)));
    assert(!mkdir("/data/spawn-native", 0700) || errno == EEXIST);
    assert(signal(SIGUSR1, SIG_IGN) != SIG_ERR);
    assert(signal(SIGUSR2, SIG_IGN) != SIG_ERR);
    sigset_t mask;
    assert(!sigemptyset(&mask));
    assert(!sigaddset(&mask, SIGUSR1) && !sigaddset(&mask, SIGUSR2));
    assert(!sigprocmask(SIG_SETMASK, &mask, NULL));
    int pair[2];
    assert(!pipe(pair));
    assert(pair[0] != 5 && pair[1] != 5);
    assert(dup2(pair[0], 30) == 30);
    pthread_t thread;
    assert(!pthread_create(&thread, NULL, worker, &thread));
    assert(fork() == -1 && errno == EBUSY);
    posix_spawnattr_t attributes;
    assert(!posix_spawnattr_init(&attributes));
    assert(!posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETSIGMASK |
                                                      POSIX_SPAWN_SETSID |
                                                      POSIX_SPAWN_CLOEXEC_DEFAULT));
    assert(!sigemptyset(&mask) && !sigaddset(&mask, SIGUSR1));
    assert(!posix_spawnattr_setsigdefault(&attributes, &mask));
    assert(!sigemptyset(&mask) && !sigaddset(&mask, SIGUSR2));
    assert(!sigaddset(&mask, SIGKILL) && !sigaddset(&mask, SIGSTOP));
    assert(!posix_spawnattr_setsigmask(&attributes, &mask));
    posix_spawn_file_actions_t actions;
    assert(!posix_spawn_file_actions_init(&actions));
    assert(!posix_spawn_file_actions_addchdir_np(&actions, "/data/spawn-native"));
    assert(!posix_spawn_file_actions_adddup2(&actions, pair[1], 31));
    assert(!posix_spawn_file_actions_addopen(&actions, 5, "child.txt", O_WRONLY | O_CREAT | O_TRUNC,
                                             0600));
    char *arguments[] = {argv[0], "child", NULL};
    char *environment[] = {"SPAWN_TEXT=ação", NULL};
    pid_t pid;
    assert(!posix_spawn(&pid, argv[0], &actions, &attributes, arguments, environment));
    int status;
    assert(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && !WEXITSTATUS(status));
    char token;
    assert(read(pair[0], &token, 1) == 1 && token == 'S');
    assert(fcntl(30, F_GETFD) == 0);
    assert(getcwd(after, sizeof(after)) && !strcmp(cwd, after));
    assert(!posix_spawn_file_actions_destroy(&actions));
    assert(!posix_spawn_file_actions_init(&actions));
    assert(!posix_spawn_file_actions_addchdir(&actions, "/data/spawn-native/absent"));
    for (unsigned i = 0; i < 64; ++i)
        assert(posix_spawn(&pid, argv[0], &actions, &attributes, arguments, environment) == ENOENT);
    assert(!posix_spawn_file_actions_destroy(&actions));
    assert(!posix_spawnattr_destroy(&attributes));
    assert(getcwd(after, sizeof(after)) && !strcmp(cwd, after));
    struct lume_spawn_options_request request = {.base = {.path = (uintptr_t)argv[0],
                                                          .argv = (uintptr_t)arguments,
                                                          .argc = 2,
                                                          .version = LUME_SPAWN_OPTIONS_VERSION},
                                                 .reserved = 1};
    assert(lume_native_call(LUME_SYS_RUN_ACTIONS, (uintptr_t)&request, sizeof(request), 0) == -22);
    request.reserved = 0;
    assert(lume_native_call(LUME_SYS_RUN_ACTIONS, (uintptr_t)&request, sizeof(request) - 1, 0) ==
           -14);
    request.flags = 0x80000000;
    assert(lume_native_call(LUME_SYS_RUN_ACTIONS, (uintptr_t)&request, sizeof(request), 0) == -22);
    assert(!pthread_mutex_lock(&lock));
    release_worker = 1;
    assert(!pthread_cond_signal(&condition) && !pthread_mutex_unlock(&lock));
    void *value;
    assert(!pthread_join(thread, &value) && value == &thread);
    assert(!close(pair[0]) && !close(pair[1]) && !close(30));
    FILE *file = fopen("/data/spawn-native/child.txt", "r");
    char data[32];
    assert(file && fgets(data, sizeof(data), file) && !strcmp(data, "SPAWN_FILE_PASS\n"));
    assert(!fclose(file) && !remove("/data/spawn-native/child.txt"));
    assert(!rmdir("/data/spawn-native"));
    puts("SPAWN_SIGNALS_CWD_FD31_WORKERS_ROLLBACK_PASS");
    return 0;
}
