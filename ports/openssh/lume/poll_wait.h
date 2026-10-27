#ifndef LUME_OPENSSH_POLL_WAIT_H
#define LUME_OPENSSH_POLL_WAIT_H
#include "lume/abi.h"
#include "lume/signal.h"
#include "fd.h"
#include <stdint.h>
#include <errno.h>

static int lume_ssh_ppoll(struct pollfd *fds, nfds_t count, const struct timespec *timeout,
                          const sigset_t *mask)
{
    if (count > LUME_FD_LIMIT || (!fds && count) ||
        (timeout &&
         (timeout->tv_sec < 0 || timeout->tv_nsec < 0 || timeout->tv_nsec >= 1000000000)))
    {
        errno = EINVAL;
        return -1;
    }
    uint64_t ms = UINT64_MAX;
    if (timeout)
    {
        uint64_t fraction = ((uint64_t)timeout->tv_nsec + 999999) / 1000000;
        if ((uint64_t)timeout->tv_sec > (UINT64_MAX - 1 - fraction) / 1000)
        {
            errno = EOVERFLOW;
            return -1;
        }
        ms = (uint64_t)timeout->tv_sec * 1000 + fraction;
    }
    struct lume_signal_request signal = {.version = LUME_SIGNAL_VERSION,
                                         .bytes = sizeof(signal),
                                         .operation = 0,
                                         .flags = mask != NULL,
                                         .mask = mask ? *mask : 0};
    if (native_result(
            native_call(LUME_SYS_SIGNAL, LUME_SIGNAL_MASK, (uintptr_t)&signal, sizeof(signal))) < 0)
        return -1;
    sigset_t old = signal.mask;
    struct lume_pollfd records[LUME_FD_LIMIT];
    for (unsigned i = 0; i < count; ++i)
    {
        records[i] = (struct lume_pollfd){.fd = fds[i].fd,
                                          .events = (fds[i].events & POLLIN ? LUME_POLLIN : 0) |
                                                    (fds[i].events & POLLOUT ? LUME_POLLOUT : 0)};
        fds[i].revents = 0;
    }
    int result;
    if (signal.interval_ms)
    {
        errno = EINTR;
        result = -1;
    }
    else
    {
        struct lume_poll_request request = {.version = LUME_POLL_VERSION,
                                            .bytes = sizeof(request),
                                            .buffer = (uintptr_t)records,
                                            .timeout_ms = ms,
                                            .signal_epoch = signal.delay_ms,
                                            .count = (uint32_t)count,
                                            .flags = LUME_POLL_SIGNAL_EPOCH};
        result = native_result(
            native_call(LUME_SYS_FD, LUME_FD_WAIT, (uintptr_t)&request, sizeof(request)));
    }
    int saved = errno;
    if (mask && sigprocmask(SIG_SETMASK, &old, NULL) < 0)
        return -1;
    errno = saved;
    if (result >= 0)
        for (unsigned i = 0; i < count; ++i)
            fds[i].revents = (records[i].revents & LUME_POLLIN ? POLLIN : 0) |
                             (records[i].revents & LUME_POLLOUT ? POLLOUT : 0) |
                             (records[i].revents & LUME_POLLERR ? POLLERR : 0) |
                             (records[i].revents & LUME_POLLHUP ? POLLHUP : 0) |
                             (records[i].revents & LUME_POLLNVAL ? POLLNVAL : 0);
    return result;
}

static int lume_ssh_poll(struct pollfd *fds, nfds_t count, int timeout)
{
    if (timeout < -1)
    {
        errno = EINVAL;
        return -1;
    }
    struct timespec delay = {.tv_sec = timeout > 0 ? timeout / 1000 : 0,
                             .tv_nsec = timeout > 0 ? (timeout % 1000) * 1000000L : 0};
    return lume_ssh_ppoll(fds, count, timeout < 0 ? NULL : &delay, NULL);
}
#endif
