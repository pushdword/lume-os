/* SPDX-License-Identifier: BSD-3-Clause
 * Private host adapters only. Unsupported services fail explicitly. */
#include "config.h"
#include "api.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/resource.h>
#include <sys/select.h>
const char *program = "htop";

uid_t getuid(void)
{
    return (uid_t)lume_syscall(LUME_UID, 0, 0, 0);
}

uid_t geteuid(void)
{
    return getuid();
}

pid_t getpid(void)
{
    return (pid_t)lume_syscall(LUME_PID, 0, 0, 0);
}

struct passwd *getpwuid(uid_t uid)
{
    static struct passwd p;
    if (uid)
    {
        errno = ENOENT;
        return NULL;
    }
    memset(&p, 0, sizeof p);
    p.pw_name = "root";
    p.pw_uid = 0;
    p.pw_gid = 0;
    p.pw_dir = "/root";
    p.pw_shell = "/bin/sh.x32";
    return &p;
}

struct passwd *getpwnam(const char *s)
{
    if (strcmp(s, "root"))
    {
        errno = ENOENT;
        return NULL;
    }
    return getpwuid(0);
}

int access(const char *p, int mode)
{
    struct stat s;
    if (stat(p, &s) < 0)
        return -1;
    if ((mode & W_OK) && !(s.st_mode & 0222))
    {
        errno = EACCES;
        return -1;
    }
    return 0;
}

int lstat(const char *p, struct stat *s)
{
    return stat(p, s);
}

ssize_t readlink(const char *p, char *b, size_t n)
{
    (void)p;
    (void)b;
    (void)n;
    errno = EINVAL;
    return -1;
}

int kill(pid_t p, int sig)
{
    (void)p;
    (void)sig;
    errno = ENOSYS;
    return -1;
}

int setpriority(int which, id_t who, int n)
{
    (void)which;
    (void)who;
    (void)n;
    errno = ENOSYS;
    return -1;
}

FILE *popen(const char *p, const char *m)
{
    (void)p;
    (void)m;
    errno = ENOSYS;
    return NULL;
}

int pclose(FILE *f)
{
    (void)f;
    errno = ENOSYS;
    return -1;
}

int getpriority(int which, id_t who)
{
    (void)which;
    (void)who;
    errno = ENOSYS;
    return -1;
}

int pipe(int f[2])
{
    (void)f;
    errno = ENOSYS;
    return -1;
}

pid_t fork(void)
{
    errno = ENOSYS;
    return -1;
}

int dup2(int a, int b)
{
    (void)a;
    (void)b;
    errno = ENOSYS;
    return -1;
}

int execlp(const char *p, const char *a, ...)
{
    (void)p;
    (void)a;
    errno = ENOSYS;
    return -1;
}

int fcntl(int f, int c, ...)
{
    (void)f;
    (void)c;
    errno = ENOSYS;
    return -1;
}

pid_t waitpid(pid_t p, int *s, int o)
{
    (void)p;
    (void)s;
    (void)o;
    errno = ECHILD;
    return -1;
}

int select(int n, fd_set *r, fd_set *w, fd_set *e, struct timeval *t)
{
    (void)n;
    (void)r;
    (void)w;
    (void)e;
    (void)t;
    errno = ENOSYS;
    return -1;
}

/* FAT has no POSIX mode bits. This tracks the process-local requested mask;
 * it never changes permissions or bypasses a kernel check. */
mode_t umask(mode_t n)
{
    static mode_t mask = 022;
    mode_t old = mask;
    mask = n;
    return old;
}

/* htop only asks realpath for HTOPRC. The kernel resolves its absolute path. */
char *realpath(const char *p, char *out)
{
    struct stat s;
    if (!p || *p != '/' || stat(p, &s) < 0)
    {
        errno = ENOENT;
        return NULL;
    }
    size_t n = strlen(p) + 1;
    if (out)
        memcpy(out, p, n);
    else
    {
        out = malloc(n);
        if (out)
            memcpy(out, p, n);
    }
    return out;
}
