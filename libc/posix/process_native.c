#include "native_posix.h"
#include "lume/process.h"
#include "lume/credential.h"
#include "lume/signal.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/wait.h>
#include "fd.h"

extern char **environ;

pid_t fork(void)
{
    fflush(stdout);
    fflush(stderr);
    return native_result(native_call(LUME_SYS_FORK, 0, 0, 0));
}

int execve(const char *path, char *const argv[], char *const envp[])
{
    struct lume_run_request request = {
        .path = (uintptr_t)path, .argv = (uintptr_t)argv, .envp = (uintptr_t)envp};
    while (argv && argv[request.argc] && request.argc < LUME_ARG_MAX)
        ++request.argc;
    while (envp && envp[request.envc] && request.envc < LUME_ENV_MAX)
        ++request.envc;
    if (!request.argc || request.argc == LUME_ARG_MAX || request.envc == LUME_ENV_MAX)
    {
        errno = E2BIG;
        return -1;
    }
    return native_result(native_call(LUME_SYS_EXEC, (uintptr_t)&request, sizeof(request), 0));
}

int execv(const char *path, char *const argv[])
{
    return execve(path, argv, environ);
}

static int exec_search(const char *file, char *const argv[])
{
    if (strchr(file, '/'))
        return execv(file, argv);
    const char *path = getenv("PATH");
    if (!path)
        path = "/data/bin:/bin:/usr/bin";
    int saved = ENOENT;
    do
    {
        const char *end = strchr(path, ':');
        size_t count = end ? (size_t)(end - path) : strlen(path);
        char name[LUME_PATH_MAX];
        if (snprintf(name, sizeof(name), "%.*s%s%s", (int)count, path, count ? "/" : "", file) >=
            (int)sizeof(name))
        {
            errno = ENAMETOOLONG;
            return -1;
        }
        execv(name, argv);
        if (errno != ENOENT && errno != ENOTDIR)
            saved = errno;
        path = end ? end + 1 : NULL;
    } while (path);
    errno = saved;
    return -1;
}

int execvp(const char *file, char *const argv[])
{
    return exec_search(file, argv);
}

static int exec_variadic(const char *path, const char *arg, va_list ap, int search)
{
    char *argv[LUME_ARG_MAX];
    unsigned count = 0;
    for (; arg && count + 1 < LUME_ARG_MAX; arg = va_arg(ap, const char *))
        argv[count++] = (char *)arg;
    if (arg)
    {
        errno = E2BIG;
        return -1;
    }
    argv[count] = NULL;
    return search ? exec_search(path, argv) : execv(path, argv);
}

int execl(const char *path, const char *arg, ...)
{
    va_list ap;
    va_start(ap, arg);
    int result = exec_variadic(path, arg, ap, 0);
    va_end(ap);
    return result;
}

int execlp(const char *path, const char *arg, ...)
{
    va_list ap;
    va_start(ap, arg);
    int result = exec_variadic(path, arg, ap, 1);
    va_end(ap);
    return result;
}

static struct lume_credential_request identity_request(void)
{
    return (struct lume_credential_request){.version = LUME_CREDENTIAL_VERSION,
                                            .bytes = sizeof(struct lume_credential_request),
                                            .uid = UINT32_MAX,
                                            .euid = UINT32_MAX,
                                            .suid = UINT32_MAX,
                                            .gid = UINT32_MAX,
                                            .egid = UINT32_MAX,
                                            .sgid = UINT32_MAX};
}

static int credential(unsigned op, struct lume_credential_request *request)
{
    return native_result(
        native_call(LUME_SYS_CREDENTIAL, op, (uintptr_t)request, sizeof(*request)));
}

int setresuid(uid_t real, uid_t effective, uid_t saved)
{
    struct lume_credential_request r = identity_request();
    r.uid = real == (uid_t)-1 ? UINT32_MAX : real;
    r.euid = effective == (uid_t)-1 ? UINT32_MAX : effective;
    r.suid = saved == (uid_t)-1 ? UINT32_MAX : saved;
    return credential(LUME_CREDENTIAL_UID, &r);
}

int setresgid(gid_t real, gid_t effective, gid_t saved)
{
    struct lume_credential_request r = identity_request();
    r.gid = real == (gid_t)-1 ? UINT32_MAX : real;
    r.egid = effective == (gid_t)-1 ? UINT32_MAX : effective;
    r.sgid = saved == (gid_t)-1 ? UINT32_MAX : saved;
    return credential(LUME_CREDENTIAL_GID, &r);
}

int seteuid(uid_t uid)
{
    return setresuid(-1, uid, -1);
}

int setegid(gid_t gid)
{
    return setresgid(-1, gid, -1);
}

int setreuid(uid_t a, uid_t b)
{
    return setresuid(a, b, b == (uid_t)-1 ? a : b);
}

int setregid(gid_t a, gid_t b)
{
    return setresgid(a, b, b == (gid_t)-1 ? a : b);
}

int setuid(uid_t uid)
{
    return geteuid() ? seteuid(uid) : setresuid(uid, uid, uid);
}

int setgid(gid_t gid)
{
    return geteuid() ? setegid(gid) : setresgid(gid, gid, gid);
}

int setgroups(int count, const gid_t *groups)
{
    struct lume_credential_request r = identity_request();
    if (count < 0 || (unsigned)count > LUME_GROUP_MAX)
    {
        errno = EINVAL;
        return -1;
    }
    r.groups_count = count;
    for (size_t i = 0; i < count; ++i)
        r.groups[i] = groups[i];
    return credential(LUME_CREDENTIAL_GROUPS, &r);
}

int getgroups(int count, gid_t *groups)
{
    struct lume_credential_request r = identity_request();
    if (credential(LUME_CREDENTIAL_GET, &r))
        return -1;
    if (count && count < (int)r.groups_count)
    {
        errno = EINVAL;
        return -1;
    }
    for (unsigned i = 0; count && i < r.groups_count; ++i)
        groups[i] = r.groups[i];
    return r.groups_count;
}

int chroot(const char *path)
{
    struct lume_credential_request r = identity_request();
    r.path = (uintptr_t)path;
    return credential(LUME_CREDENTIAL_CHROOT, &r);
}

static int resource(int resource, struct rlimit *limit, int set)
{
    unsigned index;
    switch (resource)
    {
    case RLIMIT_CORE:
        index = LUME_LIMIT_CORE_SIZE;
        break;
    case RLIMIT_NOFILE:
        index = LUME_LIMIT_FILES;
        break;
    case RLIMIT_FSIZE:
        index = LUME_LIMIT_FILE_SIZE;
        break;
    case RLIMIT_NPROC:
        index = LUME_LIMIT_PROCESSES;
        break;
    default:
        errno = EINVAL;
        return -1;
    }
    struct lume_limit_request r = {.version = LUME_CREDENTIAL_VERSION,
                                   .bytes = sizeof(r),
                                   .resource = index,
                                   .soft = limit->rlim_cur,
                                   .hard = limit->rlim_max};
    int result = native_result(native_call(
        LUME_SYS_CREDENTIAL, set ? LUME_CREDENTIAL_LIMIT_SET : LUME_CREDENTIAL_LIMIT_GET,
        (uintptr_t)&r, sizeof(r)));
    if (!result)
    {
        limit->rlim_cur = r.soft;
        limit->rlim_max = r.hard;
    }
    return result;
}

int getrlimit(int which, struct rlimit *limit)
{
    memset(limit, 0, sizeof(*limit));
    return resource(which, limit, 0);
}

int setrlimit(int which, const struct rlimit *limit)
{
    struct rlimit copy = *limit;
    return resource(which, &copy, 1);
}

int getdtablesize(void)
{
    return LUME_FD_LIMIT;
}

#include "lume/signal_native.h"

static int signal_request(unsigned operation, struct lume_signal_request *r)
{
    r->version = LUME_SIGNAL_VERSION;
    r->bytes = sizeof(*r);
    return native_result(native_call(LUME_SYS_SIGNAL, operation, (uintptr_t)r, sizeof(*r)));
}

static int process_id(unsigned operation, pid_t pid, pid_t group)
{
    struct lume_signal_request r = {.pid = pid, .group = group};
    return signal_request(operation, &r);
}

pid_t getppid(void)
{
    return process_id(LUME_SIGNAL_PARENT, 0, 0);
}

pid_t setsid(void)
{
    return process_id(LUME_SIGNAL_SET_SESSION, 0, 0);
}

pid_t getsid(pid_t pid)
{
    return process_id(LUME_SIGNAL_SESSION, pid, 0);
}

pid_t getpgid(pid_t pid)
{
    return process_id(LUME_SIGNAL_GROUP, pid, 0);
}

pid_t getpgrp(void)
{
    return getpgid(0);
}

int setpgid(pid_t pid, pid_t group)
{
    return process_id(LUME_SIGNAL_SET_GROUP, pid, group);
}

int setpgrp(void)
{
    return setpgid(0, 0);
}

int setitimer(int which, const struct itimerval *timer, struct itimerval *old)
{
    if (which != ITIMER_REAL || timer->it_value.tv_sec < 0 || timer->it_interval.tv_sec < 0 ||
        timer->it_value.tv_usec < 0 || timer->it_value.tv_usec >= 1000000 ||
        timer->it_interval.tv_usec < 0 || timer->it_interval.tv_usec >= 1000000)
    {
        errno = EINVAL;
        return -1;
    }
    struct lume_signal_request r = {.delay_ms = (uint64_t)timer->it_value.tv_sec * 1000 +
                                                (timer->it_value.tv_usec + 999) / 1000,
                                    .interval_ms = (uint64_t)timer->it_interval.tv_sec * 1000 +
                                                   (timer->it_interval.tv_usec + 999) / 1000};
    if (signal_request(LUME_SIGNAL_TIMER, &r))
        return -1;
    if (old)
    {
        old->it_value = (struct timeval){r.delay_ms / 1000, (r.delay_ms % 1000) * 1000};
        old->it_interval = (struct timeval){r.interval_ms / 1000, (r.interval_ms % 1000) * 1000};
    }
    return 0;
}
