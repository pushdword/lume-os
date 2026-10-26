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
#include <fcntl.h>
#include "lume/spawn.h"

struct __posix_spawnattr
{
    short flags;
};

struct __posix_spawn_file_actions
{
    unsigned count;
    struct lume_spawn_action action[LUME_SPAWN_ACTION_MAX];
    char *paths[LUME_SPAWN_ACTION_MAX];
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
#ifdef LUME_ARCH_AARCH64
    return (uid_t)lume_syscall(LUME_UID, 1, 0, 0);
#else
    return getuid();
#endif
}

gid_t getgid(void)
{
#ifdef LUME_ARCH_AARCH64
    return (gid_t)lume_syscall(LUME_UID, 2, 0, 0);
#else
    return 0;
#endif
}

gid_t getegid(void)
{
#ifdef LUME_ARCH_AARCH64
    return (gid_t)lume_syscall(LUME_UID, 3, 0, 0);
#else
    return getgid();
#endif
}

pid_t waitpid(pid_t pid, int *status, int options)
{
    if (options & ~WNOHANG || pid == 0 || pid < -1)
    {
        errno = EINVAL;
        return -1;
    }
#ifdef LUME_ARCH_AARCH64
    extern int32_t lume_aarch64_posix_wait(int, int *, int);
    int result = lume_aarch64_posix_wait(pid, status, !!(options & WNOHANG));
    if (result < 0)
    {
        errno = lume_errno_from_syscall((unsigned)-result);
        return -1;
    }
    return result;
#else
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
#endif
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
    if (!actions || !*actions)
        return EINVAL;
    for (unsigned i = 0; i < (*actions)->count; ++i)
        free((*actions)->paths[i]);
    free(*actions);
    *actions = NULL;
    return 0;
}

int posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *actions, int source, int target)
{
    if (!actions || !*actions || source < 0 || target < 0 || source >= 16 || target >= 16)
        return EINVAL;
    if ((*actions)->count == LUME_SPAWN_ACTION_MAX)
        return ENOMEM;
    (*actions)->action[(*actions)->count++] = (struct lume_spawn_action){
        .operation = LUME_SPAWN_DUP2, .fd = (unsigned)source, .target = (unsigned)target};
    return 0;
}

int posix_spawn_file_actions_addclose(posix_spawn_file_actions_t *actions, int fd)
{
    if (!actions || !*actions || fd < 0 || fd >= 16)
        return EINVAL;
    if ((*actions)->count == LUME_SPAWN_ACTION_MAX)
        return ENOMEM;
    (*actions)->action[(*actions)->count++] =
        (struct lume_spawn_action){.operation = LUME_SPAWN_CLOSE, .fd = (unsigned)fd};
    return 0;
}

int posix_spawn_file_actions_addopen(posix_spawn_file_actions_t *actions, int fd, const char *path,
                                     int flags, mode_t mode)
{
    (void)mode;
    if (!actions || !*actions || fd < 0 || fd >= 16 || !path)
        return EINVAL;
    unsigned allowed = O_ACCMODE | O_CREAT | O_TRUNC | O_APPEND | O_EXCL;
    if (((unsigned)flags & ~allowed) || (flags & O_ACCMODE) > O_RDWR)
        return ENOTSUP;
    if ((*actions)->count == LUME_SPAWN_ACTION_MAX)
        return ENOMEM;
    size_t bytes = strlen(path) + 1;
    if (bytes > 128)
        return ENAMETOOLONG;
    char *copy = malloc(bytes);
    if (!copy)
        return ENOMEM;
    memcpy(copy, path, bytes);
    unsigned native = flags & O_ACCMODE;
    if (flags & O_CREAT)
        native |= 0x100;
    if (flags & O_TRUNC)
        native |= 0x200;
    if (flags & O_APPEND)
        native |= 0x400;
    if (flags & O_EXCL)
        native |= 0x800;
    unsigned i = (*actions)->count++;
    (*actions)->paths[i] = copy;
    (*actions)->action[i] = (struct lume_spawn_action){
        .path = (uintptr_t)copy, .operation = LUME_SPAWN_OPEN, .fd = (unsigned)fd, .flags = native};
    return 0;
}

int posix_spawn(pid_t *pid, const char *path, const posix_spawn_file_actions_t *actions,
                const posix_spawnattr_t *attribute, char *const argv[], char *const envp[])
{
    if (!pid || !path || !argv || (actions && !*actions) ||
        (attribute && (!*attribute || (*attribute)->flags)))
        return EINVAL;
    unsigned argc = 0, envc = 0;
    while (argc < LUME_ARGS && argv[argc])
        ++argc;
    while (envp && envc < LUME_ENVS && envp[envc])
        ++envc;
    if (argv[argc] || (envp && envp[envc]))
        return E2BIG;
    int result;
    if (actions && (*actions)->count)
    {
        char *fallback[] = {(char *)path, NULL};
        struct lume_spawn_actions_request request = {.path = (uintptr_t)path,
                                                     .argv = (uintptr_t)(argc ? argv : fallback),
                                                     .argc = argc ? argc : 1,
                                                     .envp = (uintptr_t)envp,
                                                     .envc = envc,
                                                     .actions = (uintptr_t)(*actions)->action,
                                                     .count = (*actions)->count,
                                                     .version = LUME_SPAWN_VERSION};
        result =
            lume_syscall(LUME_SPAWN_ACTIONS, (uint32_t)(uintptr_t)&request, sizeof(request), 0);
    }
    else
        result =
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
