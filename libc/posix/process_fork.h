#ifndef LUME_PROCESS_FORK_H
#define LUME_PROCESS_FORK_H

pid_t fork(void)
{
    fflush(stdout);
    fflush(stderr);
    struct lume_atfork_snapshot snapshot;
    lume_atfork_prepare(&snapshot);
    lume_pthread_fork_prepare();
    int64_t result = native_call(LUME_SYS_FORK, 0, 0, 0);
    if (result == 0)
    {
        lume_pthread_fork_child();
        lume_atfork_child(&snapshot);
    }
    else
    {
        lume_pthread_fork_parent();
        lume_atfork_parent(&snapshot);
    }
    return native_result(result);
}

#endif
