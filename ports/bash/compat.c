/* SPDX-License-Identifier: BSD-3-Clause */
#include "native_posix.h"
#include "lume/signal.h"
#include "lume/credential.h"
#include <grp.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

int __real_fileno(FILE *);

int __wrap_fileno(FILE *stream)
{
    if (stream == stdin)
        return 0;
    if (stream == stdout)
        return 1;
    if (stream == stderr)
        return 2;
    return __real_fileno(stream);
}

static int wait_signal(struct lume_signal_request *request)
{
    request->version = LUME_SIGNAL_VERSION;
    request->bytes = sizeof(*request);
    int result = native_result(
        native_call(LUME_SYS_SIGNAL, LUME_SIGNAL_MASK, (uintptr_t)request, sizeof(*request)));
    if (result < 0)
        return -1;
    uint64_t epoch = request->delay_ms;
    do
    {
        result = native_result(native_call(LUME_SYS_SLEEP, 60000, epoch, 1));
    } while (result >= 0);
    return result;
}

int sigsuspend(const sigset_t *mask)
{
    struct lume_signal_request request = {.flags = 1, .mask = *mask};
    int result = wait_signal(&request);
    int saved = errno;
    if (sigprocmask(SIG_SETMASK, &request.mask, NULL) < 0)
        return -1;
    errno = saved;
    return result;
}

int pause(void)
{
    struct lume_signal_request request = {0};
    return wait_signal(&request);
}

static int identity(struct lume_credential_request *request)
{
    request->version = LUME_CREDENTIAL_VERSION;
    request->bytes = sizeof(*request);
    return native_result(native_call(LUME_SYS_CREDENTIAL, LUME_CREDENTIAL_GET, (uintptr_t)request,
                                     sizeof(*request)));
}

int getresuid(uid_t *real, uid_t *effective, uid_t *saved)
{
    struct lume_credential_request request = {0};
    if (identity(&request) < 0)
        return -1;
    *real = request.uid;
    *effective = request.euid;
    *saved = request.suid;
    return 0;
}

int getresgid(gid_t *real, gid_t *effective, gid_t *saved)
{
    struct lume_credential_request request = {0};
    if (identity(&request) < 0)
        return -1;
    *real = request.gid;
    *effective = request.egid;
    *saved = request.sgid;
    return 0;
}

int tcflow(int fd, int action)
{
    (void)fd;
    (void)action;
    errno = ENOTSUP;
    return -1;
}
