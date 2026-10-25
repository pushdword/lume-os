#define lume_syscall spawn_trace_syscall
#include "../../libc/common/posix/process.c"
#undef lume_syscall

int32_t lume_syscall(uint32_t, uint32_t, uint32_t, uint32_t);
int __real_pipe(int[2]);
int __real_posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *, int, int);
int __real_posix_spawnattr_setflags(posix_spawnattr_t *, short);

int __wrap_pipe(int pair[2])
{
    int result = __real_pipe(pair);
    printf("SPAWN_TRACE pipe=%d read=%d write=%d errno=%d\n", result, result ? -1 : pair[0],
           result ? -1 : pair[1], errno);
    return result;
}

int __wrap_posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *actions, int from, int to)
{
    int result = __real_posix_spawn_file_actions_adddup2(actions, from, to);
    printf("SPAWN_TRACE adddup2 from=%d to=%d result=%d\n", from, to, result);
    return result;
}

int __wrap_posix_spawnattr_setflags(posix_spawnattr_t *attribute, short flags)
{
    int result = __real_posix_spawnattr_setflags(attribute, flags);
    printf("SPAWN_TRACE setflags flags=%d result=%d\n", flags, result);
    return result;
}

int32_t spawn_trace_syscall(uint32_t number, uint32_t a, uint32_t b, uint32_t c)
{
    if (number == LUME_SPAWN_ACTIONS)
    {
        const struct lume_spawn_actions_request *request = (const void *)(uintptr_t)a;
        printf("SPAWN_TRACE bytes=%u version=%u argc=%u envc=%u count=%u path=%s\n", b,
               request->version, request->argc, request->envc, request->count,
               (const char *)(uintptr_t)request->path);
        const struct lume_spawn_action *actions = (const void *)(uintptr_t)request->actions;
        for (unsigned i = 0; i < request->count; i++)
            printf("SPAWN_TRACE action=%u op=%u fd=%u target=%u flags=%u path=%llu\n", i,
                   actions[i].operation, actions[i].fd, actions[i].target, actions[i].flags,
                   (unsigned long long)actions[i].path);
        fflush(stdout);
    }
    int32_t result = lume_syscall(number, a, b, c);
    if (number == LUME_SPAWN_ACTIONS)
    {
        printf("SPAWN_TRACE result=%ld\n", (long)result);
        fflush(stdout);
    }
    return result;
}
