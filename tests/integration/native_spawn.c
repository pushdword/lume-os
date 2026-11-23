/* SPDX-License-Identifier: BSD-3-Clause */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
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
#include "lume/thread.h"

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
static int release_worker;
static _Thread_local volatile int fork_tls;
static volatile int fork_private = 17;

static void check_thread_fork(int fd, int from_worker)
{
    sigset_t mask;
    assert(!sigemptyset(&mask) && !sigaddset(&mask, SIGUSR1));
    if (from_worker)
        assert(!pthread_sigmask(SIG_UNBLOCK, &mask, NULL));
    pid_t parent = getpid();
    pthread_t self = pthread_self();
    volatile int stack_value = 23;
    fork_tls = 31 + from_worker;
    pid_t pid = fork();
    assert(pid >= 0);
    if (!pid)
    {
        struct lume_thread_request stack = {
            .tid = lume_native_call(LUME_SYS_THREAD, LUME_THREAD_SELF, 0, 0)};
        uintptr_t local = (uintptr_t)&stack_value;
        if (getpid() == parent || getppid() != parent || !pthread_equal(pthread_self(), self) ||
            fork_tls != 31 + from_worker || stack_value != 23 || fork_private != 17)
            _exit(11);
        if (lume_native_call(LUME_SYS_THREAD, LUME_THREAD_STACK, (uintptr_t)&stack,
                             sizeof(stack)) ||
            local < stack.address || local - stack.address >= stack.stack_bytes)
            _exit(12);
        if (sigprocmask(SIG_SETMASK, NULL, &mask) || !sigismember(&mask, SIGUSR2) ||
            sigismember(&mask, SIGUSR1) != !from_worker)
            _exit(13);
        fork_private = 42;
        char token = from_worker ? 'W' : 'L';
        if (write(fd, &token, 1) != 1)
            _exit(14);
        _exit(0);
    }
    int status;
    assert(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && !WEXITSTATUS(status));
    assert(fork_private == 17 && fork_tls == 31 + from_worker);
}

static void *worker(void *opaque)
{
    int previous = -1;
    assert(!pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &previous));
    assert(previous == PTHREAD_CANCEL_ENABLE);
    assert(!pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &previous));
    assert(previous == PTHREAD_CANCEL_DISABLE);
    assert(!pthread_mutex_lock(&lock));
    while (!release_worker)
        assert(!pthread_cond_wait(&condition, &lock));
    assert(!pthread_mutex_unlock(&lock));
    check_thread_fork(*(int *)opaque, 1);
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

static void group_child(void)
{
    assert(getpgrp() == getpid());
}

static void check_executable_path(const char *expected)
{
    char path[PATH_MAX];
    size_t length = strlen(expected);
    assert(length >= LUME_PATH_MAX);
    assert(lume_native_call(LUME_SYS_EXEPATH, 0, (uintptr_t)path, LUME_PATH_MAX) ==
           -LUME_E2BIG);
    assert(lume_native_call(LUME_SYS_EXEPATH, 0, (uintptr_t)path, sizeof(path)) ==
           (int64_t)length);
    assert(!strcmp(path, expected));
}

static void copy_executable(const char *source, const char *destination)
{
    FILE *input = fopen(source, "rb");
    FILE *output = fopen(destination, "wb");
    assert(input && output);
    unsigned char buffer[4096];
    size_t received;
    while ((received = fread(buffer, 1, sizeof(buffer), input)))
        assert(fwrite(buffer, 1, received, output) == received);
    assert(!ferror(input) && !fclose(input) && !fclose(output));
    assert(!chmod(destination, 0700));
}

static void test_long_executable(const char *source)
{
    char directory[192], executable[224], environment_path[sizeof(directory) + 5];
    assert(snprintf(directory, sizeof(directory), "/data/spawn-native/%0150d", 0) == 169);
    assert(!mkdir(directory, 0700));
    char name[] = "long-exec";
    assert(snprintf(executable, sizeof(executable), "%s/%s", directory, name) <
           (int)sizeof(executable));
    copy_executable(source, executable);
    assert(snprintf(environment_path, sizeof(environment_path), "PATH=%s", directory) <
           (int)sizeof(environment_path));
    char *arguments[] = {name, "long-child", executable, NULL};
    char *environment[] = {environment_path, NULL};
    pid_t pid;
    int status;
    assert(!posix_spawnp(&pid, name, NULL, NULL, arguments, environment));
    assert(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && !WEXITSTATUS(status));
    assert(!setenv("PATH", directory, 1));
    pid = fork();
    assert(pid >= 0);
    if (!pid)
    {
        int previous = -1;
        assert(!pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &previous));
        assert(previous == PTHREAD_CANCEL_DISABLE);
        execvp(name, arguments);
        _exit(127);
    }
    assert(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && !WEXITSTATUS(status));
    assert(!unlink(executable) && !rmdir(directory));
    puts("SPAWN_EXEC_LONG_PATH_PASS");
}

int main(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "child"))
    {
        child();
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "group-child"))
    {
        group_child();
        return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "long-child"))
    {
        int previous = -1;
        assert(!pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &previous));
        assert(previous == PTHREAD_CANCEL_ENABLE);
        check_executable_path(argv[2]);
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
    int previous = -1;
    errno = EBUSY;
    assert(pthread_setcancelstate(-1, &previous) == EINVAL && previous == -1 && errno == EBUSY);
    assert(!pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &previous));
    assert(previous == PTHREAD_CANCEL_ENABLE);
    pthread_t thread;
    assert(!pthread_create(&thread, NULL, worker, &pair[1]));
    check_thread_fork(pair[1], 0);
    char token;
    assert(read(pair[0], &token, 1) == 1 && token == 'L');
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
    pid_t group = -1;
    assert(posix_spawnattr_setpgroup(&attributes, -1) == EINVAL);
    assert(!posix_spawnattr_setpgroup(&attributes, 0));
    assert(!posix_spawnattr_getpgroup(&attributes, &group) && !group);
    assert(!posix_spawnattr_setflags(&attributes,
                                     POSIX_SPAWN_SETPGROUP | POSIX_SPAWN_SETSIGMASK));
    char *group_arguments[] = {argv[0], "group-child", NULL};
    assert(!posix_spawn(&pid, argv[0], NULL, &attributes, group_arguments, environment));
    assert(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && !WEXITSTATUS(status));
    assert(!posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETSIGMASK |
                                                      POSIX_SPAWN_SETSID |
                                                      POSIX_SPAWN_CLOEXEC_DEFAULT));
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
    assert(!pthread_join(thread, &value) && value == &pair[1]);
    assert(read(pair[0], &token, 1) == 1 && token == 'W');
    puts("NATIVE_MULTITHREADED_FORK_PASS");
    assert(!close(pair[0]) && !close(pair[1]) && !close(30));
    FILE *file = fopen("/data/spawn-native/child.txt", "r");
    char data[32];
    assert(file && fgets(data, sizeof(data), file) && !strcmp(data, "SPAWN_FILE_PASS\n"));
    assert(!fclose(file) && !remove("/data/spawn-native/child.txt"));
    test_long_executable(argv[0]);
    assert(!pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &previous));
    assert(previous == PTHREAD_CANCEL_DISABLE);
    assert(!pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL));
    puts("PTHREAD_CANCEL_STATE_PASS");
    assert(!rmdir("/data/spawn-native"));
    puts("SPAWN_SIGNALS_CWD_FD31_WORKERS_ROLLBACK_PASS");
    return 0;
}
