#ifndef LUME_OPENSSH_POLL_H
#define LUME_OPENSSH_POLL_H
#include_next <poll.h>
#include <signal.h>
#include <time.h>
int ppoll(struct pollfd *, nfds_t, const struct timespec *, const sigset_t *);
#endif
