/* SPDX-License-Identifier: BSD-3-Clause */
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include "lume/syscall_native.h"

static volatile sig_atomic_t observed_signal;

static void signal_received(int number)
{
    observed_signal = number;
}

static int interrupted_read(int restart)
{
    int input[2], ready[2];
    if (pipe(input) || pipe(ready))
        return 1;
    struct sigaction action = {.sa_handler = signal_received, .sa_flags = restart ? SA_RESTART : 0};
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGUSR1, &action, NULL))
        return 2;
    pid_t child = fork();
    if (child < 0)
        return 3;
    if (!child)
    {
        observed_signal = 0;
        unsigned char value = 0;
        if (write(ready[1], "R", 1) != 1)
            _exit(4);
        errno = 0;
        ssize_t result = read(input[0], &value, 1);
        if (!observed_signal ||
            (restart ? result != 1 || value != 'X' : result != -1 || errno != EINTR))
            _exit(5);
        _exit(0);
    }
    unsigned char ready_byte;
    if (read(ready[0], &ready_byte, 1) != 1 || usleep(2000) || kill(child, SIGUSR1))
        return 6;
    if (restart && write(input[1], "X", 1) != 1)
        return 7;
    int status;
    if (waitpid(child, &status, 0) != child || !WIFEXITED(status) || WEXITSTATUS(status))
        return 8;
    if (close(input[0]) || close(input[1]) || close(ready[0]) || close(ready[1]))
        return 9;
    return 0;
}

int main(void)
{
    int descriptors[2];
    if (pipe2(descriptors, O_NONBLOCK))
        return 1;
    unsigned char source = 0x5a, target = 0;
    if (lume_native_call(LUME_SYS_WRITE, (unsigned)descriptors[1], (uintptr_t)&source, 1) != 1)
        return 2;
    uint64_t invalid = UINT64_C(1) << 32 | (uintptr_t)&target;
    if (lume_native_call(LUME_SYS_READ, (unsigned)descriptors[0], invalid, 1) >= 0 ||
        lume_native_call(LUME_SYS_READ, (unsigned)descriptors[0], (uintptr_t)&target, 1) != 1 ||
        target != source)
        return 3;
    if (lume_native_call(LUME_SYS_READ, (unsigned)descriptors[0], 0, 0) ||
        lume_native_call(LUME_SYS_WRITE, (unsigned)descriptors[1], 0, 0))
        return 4;
    errno = 0;
    if (read(descriptors[0], &target, 1) != -1 || errno != EAGAIN)
        return 5;
    if (close(descriptors[0]) || close(descriptors[1]))
        return 6;
    if (interrupted_read(0) || interrupted_read(1))
        return 7;
    puts("PASS_I686_DIRECT_IO");
    return 0;
}
