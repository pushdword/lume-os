#ifndef LUME_PORT_POLL_H
#define LUME_PORT_POLL_H
#include_next <poll.h>
#include <signal.h>
#include <time.h>
#ifdef __cplusplus
extern "C"
{
#endif
    int ppoll(struct pollfd *, nfds_t, const struct timespec *, const sigset_t *);
#ifdef __cplusplus
}
#endif
#endif
