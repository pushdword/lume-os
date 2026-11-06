#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include <stdio.h>
#include <spawn.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include "lume/abi.h"
#include "lume/syscall_native.h"

static sigset_t initial;
static const char *executable;

static void spawn_observer(sigset_t expected, const sigset_t *override)
{
    char encoded[32];
    snprintf(encoded, sizeof(encoded), "%lu", (unsigned long)expected);
    char *arguments[] = {(char *)executable, "child", encoded, NULL};
    posix_spawnattr_t attributes;
    assert(!posix_spawnattr_init(&attributes));
    if (override)
    {
        assert(!posix_spawnattr_setsigmask(&attributes, override));
        assert(!posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETSIGMASK));
    }
    pid_t pid;
    assert(!posix_spawn(&pid, executable, NULL, &attributes, arguments, NULL));
    assert(!posix_spawnattr_destroy(&attributes));
    int status;
    assert(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && !WEXITSTATUS(status));
}

static void *observe(void *argument)
{
    sigset_t mask;
    assert(!pthread_sigmask(SIG_SETMASK, NULL, &mask));
    assert(mask == *(sigset_t *)argument);
    return NULL;
}

static void *change(void *argument)
{
    (void)argument;
    sigset_t mask, previous;
    assert(!pthread_sigmask(SIG_SETMASK, NULL, &mask) && mask == initial);
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR2);
    assert(!pthread_sigmask(SIG_SETMASK, &mask, &previous) && previous == initial);
    assert(lume_native_call(LUME_SYS_SLEEP, 0, 0, 1) == -LUME_ENOSYS);
    spawn_observer(mask, NULL);
    spawn_observer(initial, &initial);
    pthread_t child;
    assert(!pthread_create(&child, NULL, observe, &mask));
    assert(!pthread_join(child, NULL));
    assert(!pthread_sigmask(SIG_SETMASK, NULL, &previous) && previous == mask);
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc == 3 && !strcmp(argv[1], "child"))
    {
        sigset_t observed;
        assert(!pthread_sigmask(SIG_SETMASK, NULL, &observed));
        assert(observed == (sigset_t)strtoul(argv[2], NULL, 10));
        return 0;
    }
    executable = argv[0];
    sigset_t original;
    sigemptyset(&initial);
    sigaddset(&initial, SIGUSR1);
    assert(!pthread_sigmask(SIG_SETMASK, &initial, &original));
    for (unsigned i = 0; i < 64; ++i)
    {
        pthread_t worker;
        assert(!pthread_create(&worker, NULL, change, NULL));
        assert(!pthread_join(worker, NULL));
        sigset_t observed;
        assert(!pthread_sigmask(SIG_SETMASK, NULL, &observed) && observed == initial);
    }
    assert(!pthread_sigmask(SIG_SETMASK, &original, NULL));
    puts("NATIVE_SIGNAL_MASK_PASS");
    return 0;
}
