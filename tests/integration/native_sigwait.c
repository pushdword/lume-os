/* SPDX-License-Identifier: BSD-3-Clause */
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

static volatile sig_atomic_t delivered;
static sigset_t selected;

static void handler(int number)
{
    (void)number;
    ++delivered;
}

static void *waiter(void *argument)
{
    (void)argument;
    int number = -1;
    errno = E2BIG;
    if (sigwait(&selected, &number) || number != SIGIO || errno != E2BIG)
        return (void *)(uintptr_t)1;
    if (sigwait(&selected, &number) || number != SIGUSR1 || errno != E2BIG || delivered)
        return (void *)(uintptr_t)2;
    return NULL;
}

int main(void)
{
    struct sigaction action = {.sa_handler = handler};
    if (sigaction(SIGUSR1, &action, NULL))
        return 1;
    sigemptyset(&selected);
    sigaddset(&selected, SIGUSR1);
    sigaddset(&selected, SIGIO);
    sigset_t previous;
    if (pthread_sigmask(SIG_BLOCK, &selected, &previous) || kill(getpid(), SIGUSR1) ||
        kill(getpid(), SIGIO))
        return 2;
    pthread_t thread;
    void *result;
    if (pthread_create(&thread, NULL, waiter, NULL) || pthread_join(thread, &result) || result ||
        delivered)
        return 3;
    if (kill(getpid(), SIGUSR1))
        return 4;
    errno = EBUSY;
    int number = -1;
    if (sigwait(&selected, &number) || number != SIGUSR1 || errno != EBUSY || delivered ||
        pthread_sigmask(SIG_SETMASK, &previous, NULL))
        return 4;
    puts("NATIVE_SIGWAIT_PASS");
    return 0;
}
