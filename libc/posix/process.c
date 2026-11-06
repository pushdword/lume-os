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
#include <stdbool.h>
#include "lume/spawn.h"
#include "fd.h"
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
#define SPAWN_FD_LIMIT LUME_FD_LIMIT
#else
#define SPAWN_FD_LIMIT 16
#endif

struct __posix_spawnattr
{
    short flags;
    sigset_t defaults, mask;
};

struct __posix_spawn_file_actions
{
    unsigned count;
    struct lume_spawn_action action[LUME_SPAWN_ACTION_MAX];
    char *paths[LUME_SPAWN_ACTION_MAX];
};

pid_t getpid(void)
{
    return (pid_t)lume_app_call(LUME_PID, 0, 0, 0);
}

uid_t getuid(void)
{
    return (uid_t)lume_app_call(LUME_UID, 0, 0, 0);
}

uid_t geteuid(void)
{
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64)
    return (uid_t)lume_app_call(LUME_UID, 1, 0, 0);
#else
    return getuid();
#endif
}

gid_t getgid(void)
{
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64)
    return (gid_t)lume_app_call(LUME_UID, 2, 0, 0);
#else
    return 0;
#endif
}

gid_t getegid(void)
{
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64)
    return (gid_t)lume_app_call(LUME_UID, 3, 0, 0);
#else
    return getgid();
#endif
}

pid_t waitpid(pid_t pid, int *status, int options)
{
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
    if (options & ~(WNOHANG | WUNTRACED | WCONTINUED))
#else
    if ((options & ~WNOHANG) || (pid != -1 && pid <= 0))
#endif
    {
        errno = EINVAL;
        return -1;
    }
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
    extern int32_t lume_native_wait_options(int, int *, unsigned);
    int result = lume_native_wait_options(pid, status, (unsigned)options);
    if (result < 0)
    {
        errno = lume_errno_from_syscall((unsigned)-result);
        return -1;
    }
    return result;
#else
    struct lume_wait_status child;
    int result = lume_app_call(LUME_WAITPID, (uint32_t)pid, (uintptr_t)&child,
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
    if (!attribute)
        return EINVAL;
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
    unsigned supported = 0;
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
    supported = POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETSIGMASK | POSIX_SPAWN_SETSID |
                POSIX_SPAWN_CLOEXEC_DEFAULT;
#endif
    if ((unsigned)(unsigned short)flags & ~supported)
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
    if (!actions)
        return EINVAL;
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
    if (!actions || !*actions || source < 0 || target < 0 || source >= SPAWN_FD_LIMIT ||
        target >= SPAWN_FD_LIMIT)
        return EINVAL;
    if ((*actions)->count == LUME_SPAWN_ACTION_MAX)
        return ENOMEM;
    (*actions)->action[(*actions)->count++] = (struct lume_spawn_action){
        .operation = LUME_SPAWN_DUP2, .fd = (unsigned)source, .target = (unsigned)target};
    return 0;
}

int posix_spawn_file_actions_addclose(posix_spawn_file_actions_t *actions, int fd)
{
    if (!actions || !*actions || fd < 0 || fd >= SPAWN_FD_LIMIT)
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
    if (!actions || !*actions || fd < 0 || fd >= SPAWN_FD_LIMIT || !path)
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

int posix_spawn_file_actions_addchdir(posix_spawn_file_actions_t *actions, const char *path)
{
    if (!actions || !*actions || !path)
        return EINVAL;
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
    if ((*actions)->count == LUME_SPAWN_ACTION_MAX)
        return ENOMEM;
    size_t bytes = strlen(path) + 1;
    if (bytes > 128)
        return ENAMETOOLONG;
    char *copy = malloc(bytes);
    if (!copy)
        return ENOMEM;
    memcpy(copy, path, bytes);
    unsigned i = (*actions)->count++;
    (*actions)->paths[i] = copy;
    (*actions)->action[i] =
        (struct lume_spawn_action){.path = (uintptr_t)copy, .operation = LUME_SPAWN_CHDIR};
    return 0;
#else
    return ENOTSUP;
#endif
}

int posix_spawn_file_actions_addchdir_np(posix_spawn_file_actions_t *actions, const char *path)
{
    return posix_spawn_file_actions_addchdir(actions, path);
}

int posix_spawnattr_setsigdefault(posix_spawnattr_t *attribute, const sigset_t *mask)
{
    if (!attribute || !*attribute || !mask)
        return EINVAL;
    (*attribute)->defaults = *mask;
    return 0;
}

int posix_spawnattr_getsigdefault(const posix_spawnattr_t *attribute, sigset_t *mask)
{
    if (!attribute || !*attribute || !mask)
        return EINVAL;
    *mask = (*attribute)->defaults;
    return 0;
}

int posix_spawnattr_setsigmask(posix_spawnattr_t *attribute, const sigset_t *mask)
{
    if (!attribute || !*attribute || !mask)
        return EINVAL;
    (*attribute)->mask = *mask;
    return 0;
}

int posix_spawnattr_getsigmask(const posix_spawnattr_t *attribute, sigset_t *mask)
{
    if (!attribute || !*attribute || !mask)
        return EINVAL;
    *mask = (*attribute)->mask;
    return 0;
}

int posix_spawn(pid_t *pid, const char *path, const posix_spawn_file_actions_t *actions,
                const posix_spawnattr_t *attribute, char *const argv[], char *const envp[])
{
    if (!pid || !path || !argv || (actions && !*actions) || (attribute && !*attribute))
        return EINVAL;
    unsigned argc = 0, envc = 0;
    while (argc < LUME_ARGS && argv[argc])
        ++argc;
    while (envp && envc < LUME_ENVS && envp[envc])
        ++envc;
    if (argv[argc] || (envp && envp[envc]))
        return E2BIG;
    int result;
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
    char *fallback[] = {(char *)path, NULL};
    short flags = attribute ? (*attribute)->flags : 0;
    struct lume_spawn_options_request request = {
        .base = {.path = (uintptr_t)path,
                 .argv = (uintptr_t)(argc ? argv : fallback),
                 .envp = (uintptr_t)envp,
                 .argc = argc ? argc : 1,
                 .envc = envc,
                 .actions = actions ? (uintptr_t)(*actions)->action : 0,
                 .count = actions ? (*actions)->count : 0,
                 .version = LUME_SPAWN_VERSION}};
    if (flags & POSIX_SPAWN_SETSIGDEF)
        request.flags |= LUME_SPAWN_SIGDEFAULT;
    if (flags & POSIX_SPAWN_SETSIGMASK)
        request.flags |= LUME_SPAWN_SIGMASK;
    if (flags & POSIX_SPAWN_SETSID)
        request.flags |= LUME_SPAWN_SETSID;
    if (flags & POSIX_SPAWN_CLOEXEC_DEFAULT)
        request.flags |= LUME_SPAWN_CLOEXEC_DEFAULT;
    if (attribute)
    {
        request.signal_default = (*attribute)->defaults;
        request.signal_mask = (*attribute)->mask;
    }
    bool extended = flags != 0;
    for (unsigned i = 0; i < request.base.count; ++i)
        extended |= (*actions)->action[i].operation == LUME_SPAWN_CHDIR;
    if (extended)
        request.base.version = LUME_SPAWN_OPTIONS_VERSION;
    result = lume_app_call(LUME_SPAWN_ACTIONS, (uintptr_t)&request,
                         extended ? sizeof(request) : sizeof(request.base), 0);
#else
    if (attribute && (*attribute)->flags)
        return ENOTSUP;
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
            lume_app_call(LUME_SPAWN_ACTIONS, (uintptr_t)&request, sizeof(request), 0);
    }
    else
        result =
            app_spawn_env(path, argc, (const char *const *)argv, envc, (const char *const *)envp);
#endif
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
