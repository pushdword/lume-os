/* SPDX-License-Identifier: BSD-3-Clause */
#define _GNU_SOURCE
#include <poll.h>
#include <stdint.h>
#include <errno.h>
#include "native_posix.h"
#include "lume/signal.h"
#include "fd.h"

int ppoll(struct pollfd *fds, nfds_t count, const struct timespec *timeout, const sigset_t *mask)
{
    if (count > LUME_FD_LIMIT || (!fds && count) ||
        (timeout &&
         (timeout->tv_sec < 0 || timeout->tv_nsec < 0 || timeout->tv_nsec >= 1000000000)))
    {
        errno = EINVAL;
        return -1;
    }
    uint64_t milliseconds = UINT64_MAX;
    if (timeout)
    {
        uint64_t fraction = ((uint64_t)timeout->tv_nsec + 999999) / 1000000;
        if ((uint64_t)timeout->tv_sec > (UINT64_MAX - 1 - fraction) / 1000)
        {
            errno = EOVERFLOW;
            return -1;
        }
        milliseconds = (uint64_t)timeout->tv_sec * 1000 + fraction;
    }
    struct lume_pollfd records[LUME_FD_LIMIT];
    for (unsigned i = 0; i < count; ++i)
    {
        records[i] = (struct lume_pollfd){.fd = fds[i].fd,
                                          .events = (fds[i].events & POLLIN ? LUME_POLLIN : 0) |
                                                    (fds[i].events & POLLOUT ? LUME_POLLOUT : 0)};
        fds[i].revents = 0;
    }
    struct lume_signal_request signal = {.version = LUME_SIGNAL_VERSION,
                                         .bytes = sizeof(signal),
                                         .operation = 0,
                                         .flags = 1,
                                         .mask = mask ? *mask : 0};
    if (mask && native_result(native_call(LUME_SYS_SIGNAL, LUME_SIGNAL_MASK, (uintptr_t)&signal,
                                          sizeof(signal))) < 0)
        return -1;
    struct lume_poll_request request = {.version = LUME_POLL_VERSION,
                                        .bytes = sizeof(request),
                                        .buffer = (uintptr_t)records,
                                        .timeout_ms = milliseconds,
                                        .count = (unsigned)count,
                                        .signal_epoch = mask ? signal.delay_ms : 0,
                                        .flags = mask ? LUME_POLL_SIGNAL_EPOCH : 0};
    int ready;
    if (mask && signal.interval_ms)
    {
        errno = EINTR;
        ready = -1;
    }
    else
        ready = native_result(
            native_call(LUME_SYS_FD, LUME_FD_WAIT, (uintptr_t)&request, sizeof(request)));
    int saved = errno;
    if (mask)
    {
        sigset_t old = signal.mask;
        if (sigprocmask(SIG_SETMASK, &old, NULL))
            return -1;
    }
    errno = saved;
    if (ready >= 0)
        for (unsigned i = 0; i < count; ++i)
            fds[i].revents = (records[i].revents & LUME_POLLIN ? POLLIN : 0) |
                             (records[i].revents & LUME_POLLOUT ? POLLOUT : 0) |
                             (records[i].revents & LUME_POLLERR ? POLLERR : 0) |
                             (records[i].revents & LUME_POLLHUP ? POLLHUP : 0) |
                             (records[i].revents & LUME_POLLNVAL ? POLLNVAL : 0);
    return ready;
}
