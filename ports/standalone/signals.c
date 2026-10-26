#include "native.h"
#include <stddef.h>
#include <signal.h>
#include <lume/signal.h>

#include <lume/signal_native.h>

static int signal_request(unsigned operation, struct lume_signal_request *request)
{
    request->version = LUME_SIGNAL_VERSION;
    request->bytes = sizeof(*request);
    return native_result(LUME_SYS_SIGNAL, operation, (uintptr_t)request, sizeof(*request));
}

int sigaction(int number, const struct sigaction *action, struct sigaction *old)
{
    struct lume_signal_request request = {.number = number, .operation = action != NULL};
    if (action)
    {
        request.handler = (uintptr_t)action->sa_handler;
        request.restorer = (uintptr_t)signal_restorer;
        request.mask = action->sa_mask;
        request.flags = action->sa_flags;
    }
    if (signal_request(LUME_SIGNAL_ACTION, &request))
        return -1;
    if (old)
    {
        old->sa_handler = (void (*)(int))(uintptr_t)request.handler;
        old->sa_mask = request.mask;
        old->sa_flags = request.flags;
    }
    return 0;
}

int sigprocmask(int operation, const sigset_t *set, sigset_t *old)
{
    struct lume_signal_request request = {
        .operation = operation, .flags = set != NULL, .mask = set ? *set : 0};
    if (signal_request(LUME_SIGNAL_MASK, &request))
        return -1;
    if (old)
        *old = request.mask;
    return 0;
}

unsigned alarm(unsigned seconds)
{
    struct lume_signal_request request = {.delay_ms = (uint64_t)seconds * 1000u};
    if (signal_request(LUME_SIGNAL_TIMER, &request))
        return 0;
    return (unsigned)(request.delay_ms / 1000u + !!(request.delay_ms % 1000u));
}
