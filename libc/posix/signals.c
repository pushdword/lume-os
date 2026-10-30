#include "native_posix.h"
#include <signal.h>
#include <unistd.h>
#include <setjmp.h>
#include "lume/signal.h"
#include "lume/signal_native.h"

static int signal_call(unsigned operation, struct lume_signal_request *r)
{
    r->version = LUME_SIGNAL_VERSION;
    r->bytes = sizeof(*r);
    return native_result(native_call(LUME_SYS_SIGNAL, operation, (uintptr_t)r, sizeof(*r)));
}

int sigaction(int number, const struct sigaction *action, struct sigaction *old)
{
    struct lume_signal_request r = {.number = number, .operation = action != NULL};
    if (action)
    {
        r.handler = (uintptr_t)action->sa_handler;
        r.restorer = (uintptr_t)signal_restorer;
        r.mask = action->sa_mask;
        r.flags = action->sa_flags;
    }
    if (signal_call(LUME_SIGNAL_ACTION, &r))
        return -1;
    if (old)
        *old = (struct sigaction){.sa_handler = (sighandler_t)(uintptr_t)r.handler,
                                  .sa_mask = r.mask,
                                  .sa_flags = r.flags};
    return 0;
}

int sigprocmask(int operation, const sigset_t *set, sigset_t *old)
{
    struct lume_signal_request r = {
        .operation = operation, .flags = set != NULL, .mask = set ? *set : 0};
    if (signal_call(LUME_SIGNAL_MASK, &r))
        return -1;
    if (old)
        *old = r.mask;
    return 0;
}

sighandler_t signal(int number, sighandler_t handler)
{
    struct sigaction action = {.sa_handler = handler, .sa_flags = SA_RESTART}, old;
    return sigaction(number, &action, &old) ? SIG_ERR : old.sa_handler;
}

int sigemptyset(sigset_t *set)
{
    *set = 0;
    return 0;
}

int sigfillset(sigset_t *set)
{
    *set = UINT32_MAX & ~1u;
    return 0;
}

static int valid_signal(int number)
{
    if (number > 0 && number < NSIG)
        return 1;
    errno = EINVAL;
    return 0;
}

int sigaddset(sigset_t *set, int number)
{
    if (!valid_signal(number))
        return -1;
    *set |= UINT32_C(1) << number;
    return 0;
}

int sigdelset(sigset_t *set, int number)
{
    if (!valid_signal(number))
        return -1;
    *set &= ~(UINT32_C(1) << number);
    return 0;
}

int sigismember(const sigset_t *set, int number)
{
    return valid_signal(number) ? !!(*set & (UINT32_C(1) << number)) : -1;
}

int kill(pid_t pid, int number)
{
    struct lume_signal_request r = {.pid = pid, .number = number};
    return signal_call(LUME_SIGNAL_SEND, &r);
}

int raise(int number)
{
    return kill(getpid(), number);
}

unsigned alarm(unsigned seconds)
{
    struct lume_signal_request r = {.delay_ms = (uint64_t)seconds * 1000};
    if (signal_call(LUME_SIGNAL_TIMER, &r))
        return 0;
    return (unsigned)(r.delay_ms / 1000 + !!(r.delay_ms % 1000));
}

void siglongjmp(sigjmp_buf buffer, int value)
{
    if (buffer[0].saved)
        sigprocmask(SIG_SETMASK, &buffer[0].mask, NULL);
    longjmp(buffer[0].registers, value);
}
