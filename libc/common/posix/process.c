#include "api.h"
#include "errno_map.h"
#include <spawn.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

struct __posix_spawnattr
{
    short flags;
};

struct __posix_spawn_file_actions
{
    unsigned count;
};

pid_t getpid(void)
{
    return (pid_t)lume_syscall(LUME_PID, 0, 0, 0);
}

uid_t getuid(void)
{
    return (uid_t)lume_syscall(LUME_UID, 0, 0, 0);
}

uid_t geteuid(void)
{
    return getuid();
}

gid_t getgid(void)
{
    return 0;
}

gid_t getegid(void)
{
    return getgid();
}

pid_t waitpid(pid_t pid, int *status, int options)
{
    if (options & ~WNOHANG || pid == 0 || pid < -1)
    {
        errno = EINVAL;
        return -1;
    }
    struct lume_wait_status child;
    int result = lume_syscall(LUME_WAITPID, (uint32_t)pid, (uint32_t)(uintptr_t)&child,
                              !!(options & WNOHANG));
    if (result < 0)
    {
        errno = lume_errno_from_syscall((unsigned)-result);
        return -1;
    }
    if (result && status)
        *status = (int)((child.code & 255) << 8);
    return result;
}

pid_t wait(int *status)
{
    return waitpid(-1, status, 0);
}

int posix_spawnattr_init(posix_spawnattr_t *attribute)
{
    *attribute = calloc(1, sizeof(**attribute));
    return *attribute ? 0 : ENOMEM;
}

int posix_spawnattr_destroy(posix_spawnattr_t *attribute)
{
    free(*attribute);
    *attribute = NULL;
    return 0;
}

int posix_spawnattr_setflags(posix_spawnattr_t *attribute, short flags)
{
    if (!attribute || !*attribute)
        return EINVAL;
    if (flags)
        return ENOTSUP;
    (*attribute)->flags = flags;
    return 0;
}

int posix_spawnattr_getflags(const posix_spawnattr_t *attribute, short *flags)
{
    if (!attribute || !*attribute || !flags)
        return EINVAL;
    *flags = (*attribute)->flags;
    return 0;
}

int posix_spawn_file_actions_init(posix_spawn_file_actions_t *actions)
{
    *actions = calloc(1, sizeof(**actions));
    return *actions ? 0 : ENOMEM;
}

int posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *actions)
{
    free(*actions);
    *actions = NULL;
    return 0;
}

int posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *actions, int source, int target)
{
    if (!actions || !*actions || source < 0 || target < 0)
        return EINVAL;
    return source == target ? 0 : ENOTSUP;
}

int posix_spawn(pid_t *pid, const char *path, const posix_spawn_file_actions_t *actions,
                const posix_spawnattr_t *attribute, char *const argv[], char *const envp[])
{
    if (!pid || !path || !argv || (actions && (!*actions || (*actions)->count)) ||
        (attribute && (!*attribute || (*attribute)->flags)))
        return EINVAL;
    unsigned argc = 0, envc = 0;
    while (argv[argc] && argc < LUME_ARGS)
        ++argc;
    while (envp && envp[envc] && envc < LUME_ENVS)
        ++envc;
    if (argv[argc] || (envp && envp[envc]))
        return E2BIG;
    int result =
        app_spawn_env(path, argc, (const char *const *)argv, envc, (const char *const *)envp);
    if (result < 0)
        return lume_errno_from_syscall((unsigned)-result);
    *pid = result;
    return 0;
}

int posix_spawnp(pid_t *pid, const char *file, const posix_spawn_file_actions_t *actions,
                 const posix_spawnattr_t *attribute, char *const argv[], char *const envp[])
{
    if (strchr(file, '/'))
        return posix_spawn(pid, file, actions, attribute, argv, envp);
    const char *path = NULL;
    for (unsigned i = 0; envp && envp[i]; ++i)
        if (!strncmp(envp[i], "PATH=", 5))
            path = envp[i] + 5;
    if (!path)
        path = "/bin:/usr/bin";
    int result = ENOENT;
    while (*path)
    {
        const char *end = strchr(path, ':');
        size_t bytes = end ? (size_t)(end - path) : strlen(path);
        char name[128];
        int length = bytes ? snprintf(name, sizeof(name), "%.*s/%s", (int)bytes, path, file)
                           : snprintf(name, sizeof(name), "./%s", file);
        if (length < 0 || (unsigned)length >= sizeof(name))
            return ENAMETOOLONG;
        struct stat metadata;
        if (!stat(name, &metadata))
            return posix_spawn(pid, name, actions, attribute, argv, envp);
        if (errno != ENOENT && errno != ENOTDIR)
            result = errno;
        if (!end)
            break;
        path = end + 1;
    }
    return result;
}
