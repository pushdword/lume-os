#include "config.h"
#include "platform.h"
#include "api.h"
#include "fd_client.h"
#include "crypto_client.h"
#include "net_client.h"
#include "errno_map.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/random.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <signal.h>
#include <time.h>
#include "profile.h"
#include "terminal.h"
#ifdef LUME_SSH_PROFILE
static int measured_io(unsigned op, int fd, void *data, unsigned n)
{
    unsigned t = ssh_profile_now();
    int r = net_io(op, fd, data, n);
    ssh_profile_record(SSH_P_NETIO, ssh_profile_now() - t);
    return r;
}

static void measured_idle(unsigned ms)
{
    unsigned t = ssh_profile_now();
    net_idle(ms);
    ssh_profile_record(SSH_P_SLEEP, ssh_profile_now() - t);
}

#define net_io measured_io
#define net_idle measured_idle
#endif
/* Network descriptors are process-local adapters to kernel service37. They
 * are never inherited; session children receive kernel pipes through service39. */
struct sock
{
    int handle, flags;
    unsigned port;
    struct sockaddr_in peer;
    unsigned rx_start, rx_used;
    unsigned char rx[1024];
};
static struct sock sockets[8];

static int neterror(unsigned n)
{
    switch (n)
    {
    case 90:
        return EMSGSIZE;
    case 98:
        return EADDRINUSE;
    case 101:
        return ENETUNREACH;
    case 104:
        return ECONNRESET;
    case 107:
        return ENOTCONN;
    case 111:
        return ECONNREFUSED;
    case 114:
        return EALREADY;
    case 115:
        return EINPROGRESS;
    default:
        return lume_errno_from_syscall(n);
    }
}

static int result(int r)
{
    if (r < 0)
    {
        errno = neterror((unsigned)-r);
        return -1;
    }
    return r;
}

static struct sock *sock(int fd)
{
    return fd >= 32 && fd < 40 && sockets[fd - 32].handle ? &sockets[fd - 32] : NULL;
}

static int install(int handle)
{
    for (int i = 0; i < 8; i++)
        if (!sockets[i].handle)
        {
            sockets[i] = (struct sock){.handle = handle};
            return i + 32;
        }
    net_close(handle);
    errno = EMFILE;
    return -1;
}

int socket(int family, int type, int protocol)
{
    if (family != AF_INET || type != SOCK_STREAM || (protocol && protocol != IPPROTO_TCP))
    {
        errno = EAFNOSUPPORT;
        return -1;
    }
    int r = net_socket(LUME_NET_TCP);
    return r < 0 ? result(r) : install(r);
}

int bind(int fd, const struct sockaddr *address, socklen_t size)
{
    struct sock *s = sock(fd);
    if (!s)
    {
        errno = EBADF;
        return -1;
    }
    if (!address || size < sizeof(struct sockaddr_in) || address->sa_family != AF_INET)
    {
        errno = EINVAL;
        return -1;
    }
    const struct sockaddr_in *a = (const void *)address;
    struct lume_net_info info;
    if (a->sin_addr.s_addr &&
        (net_info(&info) < 0 || memcmp(&a->sin_addr, info.configuration.address, 4)))
    {
        errno = EADDRNOTAVAIL;
        return -1;
    }
    int r = net_bind(s->handle, ntohs(a->sin_port));
    if (!r)
        s->port = ntohs(a->sin_port);
    return result(r);
}

int listen(int fd, int backlog)
{
    (void)backlog;
    struct sock *s = sock(fd);
    if (!s)
    {
        errno = EBADF;
        return -1;
    }
    int n = net_listen(s->handle);
    if (n < 0)
        return result(n);
    /* POSIX listen must finish before SSH_READY is advertised. Raw service37
  * starts OPEN asynchronously; early SYNs can otherwise hit SOCK_INIT and
  * require a client retransmission. Only the kernel reads the W5500 status. */
    uint32_t started = net_now();
    for (;;)
    {
        struct lume_net_request r = net_request(s->handle);
        r.events = LUME_NET_POLLLISTEN;
        n = net_call(LUME_NET_POLL, &r);
        if (n < 0)
            return result(n);
        if (r.events & (LUME_NET_POLLERR | LUME_NET_POLLHUP))
        {
            errno = EIO;
            return -1;
        }
        if (r.events & LUME_NET_POLLLISTEN)
            return 0;
        if ((uint32_t)(net_now() - started) >= 10000)
        {
            errno = ETIMEDOUT;
            return -1;
        }
        net_idle(5);
    }
}

static int address_copy(const struct sockaddr_in *from, struct sockaddr *to, socklen_t *size)
{
    if (to)
    {
        if (!size)
        {
            errno = EFAULT;
            return -1;
        }
        unsigned n = *size < sizeof *from ? *size : sizeof *from;
        memcpy(to, from, n);
        *size = sizeof *from;
    }
    return 0;
}

int accept(int fd, struct sockaddr *address, socklen_t *size)
{
    struct sock *s = sock(fd);
    if (!s)
    {
        errno = EBADF;
        return -1;
    }
    for (;;)
    {
        struct lume_net_request r = net_request(s->handle);
        int n = net_call(LUME_NET_ACCEPT, &r);
        if (n >= 0)
        {
            int out = install(n);
            if (out < 0)
                return -1;
            struct sock *child = sock(out);
            child->port = s->port;
            child->peer.sin_family = AF_INET;
            child->peer.sin_port = htons(r.port);
            memcpy(&child->peer.sin_addr, r.address, 4);
            if (address_copy(&child->peer, address, size) < 0)
            {
                ssh_fdclose(out);
                return -1;
            }
            return out;
        }
        if (n != -11 || s->flags & O_NONBLOCK)
            return result(n);
        net_idle(5);
    }
}

int connect(int fd, const struct sockaddr *address, socklen_t size)
{
    struct sock *s = sock(fd);
    if (!s)
    {
        errno = EBADF;
        return -1;
    }
    if (!address || size < sizeof(struct sockaddr_in) || address->sa_family != AF_INET)
    {
        errno = EINVAL;
        return -1;
    }
    s->peer = *(const struct sockaddr_in *)(const void *)address;
    struct lume_net_request r = net_request(s->handle);
    r.port = ntohs(s->peer.sin_port);
    memcpy(r.address, &s->peer.sin_addr, 4);
    return result(net_call(LUME_NET_CONNECT, &r));
}

int ssh_fdread(int fd, void *data, unsigned size)
{
    struct sock *s = sock(fd);
    if (!s)
        return ssh_terminal_read(fd, data, size);
    if (!size)
        return 0;
    if (size > 1024)
        size = 1024;
    for (;;)
    {
        if (!s->rx_used)
        {
            int r = net_io(LUME_NET_RECV, s->handle, s->rx, sizeof s->rx);
            if (r < 0)
            {
                if (r == -11 && !(s->flags & O_NONBLOCK))
                {
                    net_idle(5);
                    continue;
                }
                return -neterror(-r);
            }
            if (!r)
                return 0;
            s->rx_start = 0;
            s->rx_used = (unsigned)r;
        }
        if (size > s->rx_used)
            size = s->rx_used;
        memcpy(data, s->rx + s->rx_start, size);
        s->rx_start += size;
        s->rx_used -= size;
        return (int)size;
    }
}

int ssh_fdwrite(int fd, const void *data, unsigned size)
{
    struct sock *s = sock(fd);
    if (!s)
        return lume_posix_write(fd, data, size);
    if (!size)
        return 0;
    if (size > 1024)
        size = 1024;
    for (;;)
    {
        int r = net_io(LUME_NET_SEND, s->handle, (void *)data, size);
        if (r != -11 || s->flags & O_NONBLOCK)
            return r < 0 ? -neterror(-r) : r;
        net_idle(5);
    }
}

int ssh_fdclose(int fd)
{
    struct sock *s = sock(fd);
    if (!s)
    {
        int r = lume_posix_close(fd);
        if (!r)
            ssh_terminal_forget(fd);
        return r;
    }
    int r = net_close(s->handle);
    if (!r)
        memset(s, 0, sizeof *s);
    return r < 0 ? -neterror(-r) : r;
}

ssize_t recv(int fd, void *p, size_t n, int flags)
{
    if (flags)
    {
        errno = EOPNOTSUPP;
        return -1;
    }
    int r = ssh_fdread(fd, p, n);
    if (r < 0)
    {
        errno = -r;
        return -1;
    }
    return r;
}

ssize_t send(int fd, const void *p, size_t n, int flags)
{
    if (flags & ~MSG_NOSIGNAL)
    {
        errno = EOPNOTSUPP;
        return -1;
    }
    int r = ssh_fdwrite(fd, p, n);
    if (r < 0)
    {
        errno = -r;
        return -1;
    }
    return r;
}

int getsockname(int fd, struct sockaddr *address, socklen_t *size)
{
    struct sock *s = sock(fd);
    if (!s)
    {
        errno = EBADF;
        return -1;
    }
    struct lume_net_info info;
    int r = net_info(&info);
    if (r < 0)
        return result(r);
    struct sockaddr_in a = {.sin_family = AF_INET, .sin_port = htons(s->port)};
    memcpy(&a.sin_addr, info.configuration.address, 4);
    return address_copy(&a, address, size);
}

int getpeername(int fd, struct sockaddr *address, socklen_t *size)
{
    struct sock *s = sock(fd);
    if (!s)
    {
        errno = EBADF;
        return -1;
    }
    if (!s->peer.sin_port)
    {
        errno = ENOTCONN;
        return -1;
    }
    return address_copy(&s->peer, address, size);
}

int setsockopt(int fd, int level, int name, const void *value, socklen_t size)
{
    if (!sock(fd))
    {
        errno = EBADF;
        return -1;
    }
    if (!value || size != sizeof(int))
    {
        errno = EINVAL;
        return -1;
    }
    if ((level == SOL_SOCKET && (name == SO_REUSEADDR || name == SO_KEEPALIVE)) ||
        (level == IPPROTO_TCP && name == TCP_NODELAY) || (level == IPPROTO_IP && name == IP_TOS))
        return 0;
    errno = ENOPROTOOPT;
    return -1;
}

int getsockopt(int fd, int level, int name, void *value, socklen_t *size)
{
    if (!sock(fd))
    {
        errno = EBADF;
        return -1;
    }
    if (level != SOL_SOCKET || !value || !size || *size < sizeof(int))
    {
        errno = EINVAL;
        return -1;
    }
    if (name != SO_ERROR && name != SO_TYPE)
    {
        errno = ENOPROTOOPT;
        return -1;
    }
    *(int *)value = name == SO_TYPE ? SOCK_STREAM : 0;
    *size = sizeof(int);
    return 0;
}

int shutdown(int fd, int how)
{
    (void)fd;
    (void)how;
    errno = EOPNOTSUPP;
    return -1;
}

int socketpair(int family, int type, int protocol, int pair[2])
{
    if (family != AF_UNIX || type != SOCK_STREAM || protocol)
    {
        errno = EOPNOTSUPP;
        return -1;
    }
    return result(app_fd_pair(pair, 1, 0));
}

int pipe(int pair[2])
{
    return result(app_fd_pair(pair, 0, 0));
}

int dup(int fd)
{
    if (sock(fd))
    {
        errno = EOPNOTSUPP;
        return -1;
    }
    return result(app_fd_dup(fd, 0, 0, 0));
}

int dup2(int from, int to)
{
    if (sock(from) || to >= 32)
    {
        errno = EOPNOTSUPP;
        return -1;
    }
    return result(app_fd_dup(from, to, 1, 0));
}

int fcntl(int fd, int command, ...)
{
    unsigned arg = 0;
    if (command == F_SETFL || command == F_SETFD || command == F_DUPFD)
    {
        va_list ap;
        va_start(ap, command);
        arg = (unsigned)va_arg(ap, int);
        va_end(ap);
    }
    struct sock *s = sock(fd);
    if (s)
    {
        switch (command)
        {
        case F_GETFL:
            return O_RDWR | s->flags;
        case F_SETFL:
            s->flags = arg & O_NONBLOCK;
            return 0;
        case F_GETFD:
            return FD_CLOEXEC;
        case F_SETFD:
            if (arg == FD_CLOEXEC)
                return 0;
            errno = EOPNOTSUPP;
            return -1;
        default:
            errno = EINVAL;
            return -1;
        }
    }
    int r;
    switch (command)
    {
    case F_GETFL:
        r = app_fd_flags(fd, LUME_FD_GETFL, 0);
        return r < 0 ? result(r) : ((r & LUME_FD_NONBLOCK) ? O_NONBLOCK : 0);
    case F_SETFL:
        return result(app_fd_flags(fd, LUME_FD_SETFL, (arg & O_NONBLOCK) ? LUME_FD_NONBLOCK : 0));
    case F_GETFD:
        r = app_fd_flags(fd, LUME_FD_GETFD, 0);
        return r < 0 ? result(r) : ((r & LUME_FD_CLOEXEC) ? FD_CLOEXEC : 0);
    case F_SETFD:
        return result(app_fd_flags(fd, LUME_FD_SETFD, (arg & FD_CLOEXEC) ? LUME_FD_CLOEXEC : 0));
    case F_DUPFD:
        return result(app_fd_dup(fd, arg, 0, 0));
    default:
        errno = EINVAL;
        return -1;
    }
}

int ssh_nonblock(int fd)
{
    int f = fcntl(fd, F_GETFL);
    return f >= 0 && !!(f & O_NONBLOCK);
}

int ssh_isatty(int fd)
{
    struct lume_stat s;
    if (sock(fd))
        return 0;
    if (lume_syscall(LUME_FSTAT, fd, (uint32_t)(uintptr_t)&s, 0) < 0)
        return 0;
    return (s.mode & 0170000) == 0020000;
}

int ssh_child_pid;

static int child_state(struct lume_process_info *p)
{
    static unsigned slot = 8;
    static int cached_pid;
    static uint32_t owner;
    if (!ssh_child_pid)
        return 0;
    if (!owner)
        owner = (uint32_t)lume_syscall(LUME_PID, 0, 0, 0);
    if (cached_pid == ssh_child_pid && slot < 8)
    {
        if (lume_syscall(LUME_PS, slot, (uint32_t)(uintptr_t)p, 0) > 0 &&
            p->pid == (uint32_t)ssh_child_pid && p->parent == owner)
            return 1;
    }
    for (unsigned i = 0; i < 8; i++)
        if (lume_syscall(LUME_PS, i, (uint32_t)(uintptr_t)p, 0) > 0 &&
            p->pid == (uint32_t)ssh_child_pid && p->parent == owner)
        {
            slot = i;
            cached_pid = ssh_child_pid;
            return 1;
        }
    cached_pid = 0;
    slot = 8;
    return 0;
}

static int child_zombie(void)
{
    struct lume_process_info p;
    return child_state(&p) && p.state == LUME_ZOMBIE;
}

static int select_body(int limit, fd_set *rd, fd_set *wr, fd_set *ex, struct timeval *timeout)
{
    if (limit < 0 || limit > 40 ||
        (timeout && (timeout->tv_sec < 0 || timeout->tv_usec < 0 || timeout->tv_usec >= 1000000)))
    {
        errno = EINVAL;
        return -1;
    }
    fd_set inr = {0}, inw = {0}, ine = {0};
    if (rd)
        inr = *rd;
    if (wr)
        inw = *wr;
    if (ex)
        ine = *ex;
    struct lume_pollfd poll[LUME_FD_LIMIT];
    unsigned count = 0;
    for (int fd = 0; fd < limit; fd++)
    {
        unsigned input = rd && FD_ISSET(fd, &inr), output = wr && FD_ISSET(fd, &inw),
                 error = ex && FD_ISSET(fd, &ine);
        if ((input || output || error) && !sock(fd))
        {
            if (fd >= (int)LUME_FD_LIMIT)
            {
                errno = EBADF;
                return -1;
            }
            poll[count++] = (struct lume_pollfd){
                fd, (uint16_t)((input ? LUME_POLLIN : 0) | (output ? LUME_POLLOUT : 0)), 0};
        }
    }
    uint64_t wait = timeout ? (uint64_t)timeout->tv_sec * 1000u + (timeout->tv_usec + 999u) / 1000u
                            : UINT64_MAX;
    uint32_t started = wait ? net_now() : 0, next_child = started;
    for (;;)
    {
        int ready = 0;
        if (rd)
            FD_ZERO(rd);
        if (wr)
            FD_ZERO(wr);
        if (ex)
            FD_ZERO(ex);
        /* Pipes and terminal descriptors already support a vector kernel poll.
   * Keep one call per scan, including exception-only descriptors, so EBADF
   * and HUP still have the same ordering as the socket probes below. */
        if (count)
        {
            int n = app_poll(poll, count, 0);
            if (n < 0)
                return result(n);
        }
        unsigned polled = 0;
        for (int fd = 0; fd < limit; fd++)
        {
            unsigned wantin = rd && FD_ISSET(fd, &inr), wantout = wr && FD_ISSET(fd, &inw),
                     wantex = ex && FD_ISSET(fd, &ine);
            if (!(wantin || wantout || wantex))
                continue;
            unsigned input = 0, output = 0, error = 0;
            struct sock *s = sock(fd);
            if (s)
            {
                if (s->rx_used && wantin && !wantout && !wantex)
                    input = 1;
                else
                {
                    struct lume_net_request r = net_request(s->handle);
                    r.events = (wantin ? LUME_NET_POLLIN : 0) | (wantout ? LUME_NET_POLLOUT : 0);
                    int n = net_call(LUME_NET_POLL, &r);
                    if (n < 0)
                        return result(n);
                    input = s->rx_used || !!(r.events & (LUME_NET_POLLIN | LUME_NET_POLLHUP));
                    output = !!(r.events & LUME_NET_POLLOUT);
                    error = !!(r.events & LUME_NET_POLLERR);
                }
            }
            else
            {
                unsigned events = poll[polled++].revents;
                if (events & LUME_POLLNVAL)
                {
                    errno = EBADF;
                    return -1;
                }
                input = ssh_terminal_pending(fd) || !!(events & (LUME_POLLIN | LUME_POLLHUP));
                output = !!(events & LUME_POLLOUT);
                error = !!(events & LUME_POLLERR);
            }
            if (wantin && input)
            {
                FD_SET(fd, rd);
                ready++;
            }
            if (wantout && output)
            {
                FD_SET(fd, wr);
                ready++;
            }
            if (wantex && error)
            {
                FD_SET(fd, ex);
                ready++;
            }
        }
        if (ready)
            return ready;
        if (!wait)
            return 0;
        uint32_t now = net_now();
        if (timeout && (uint32_t)(now - started) >= wait)
            return 0;
        if ((int32_t)(now - next_child) >= 0)
        {
            next_child = now + 50;
            if (child_zombie())
            {
                errno = EINTR;
                return -1;
            }
        }
        net_idle(5);
    }
}

int select(int limit, fd_set *rd, fd_set *wr, fd_set *ex, struct timeval *timeout)
{
    unsigned t = ssh_profile_now();
    int r = select_body(limit, rd, wr, ex, timeout);
    ssh_profile_record(SSH_P_SELECT, ssh_profile_now() - t);
    return r;
}

ssize_t writev(int fd, const struct iovec *v, int count)
{
    if (count < 0 || count > 1024)
    {
        errno = EINVAL;
        return -1;
    }
    if (!count)
        return 0;
    if (!v)
    {
        errno = EFAULT;
        return -1;
    }
    if (sock(fd))
    {
        /* Coalesce queued encrypted packets into one existing service37 transfer.
   * Returning a prefix is valid writev semantics; Dropbear retains its tail. */
        unsigned char block[1024];
        unsigned used = 0;
        for (int i = 0; i < count && used < sizeof block; i++)
        {
            size_t n = v[i].iov_len;
            if (n > sizeof block - used)
                n = sizeof block - used;
            if (n)
            {
                if (!v[i].iov_base)
                {
                    errno = EFAULT;
                    return -1;
                }
                memcpy(block + used, v[i].iov_base, n);
                used += (unsigned)n;
            }
        }
        return used ? write(fd, block, used) : 0;
    }
    ssize_t total = 0;
    for (int i = 0; i < count; i++)
    {
        ssize_t n = write(fd, v[i].iov_base, v[i].iov_len);
        if (n < 0)
            return total ? total : -1;
        total += n;
        if ((size_t)n < v[i].iov_len)
            break;
    }
    return total;
}

pid_t waitpid(pid_t pid, int *status, int options)
{
    if (options & ~WNOHANG)
    {
        errno = EINVAL;
        return -1;
    }
    /* This forkless port creates one tracked child through ssh_spawn_command.
  * Before a channel exists there is no child to query or reap. */
    if (options & WNOHANG)
    {
        struct lume_process_info p;
        if (!ssh_child_pid || (pid != -1 && pid != ssh_child_pid) || !child_state(&p))
        {
            errno = ECHILD;
            return -1;
        }
        if (p.state != LUME_ZOMBIE)
            return 0;
        pid = p.pid;
    }
    struct lume_wait_status s;
    int r = app_wait(pid, &s);
    if (r < 0)
        return result(r);
    if (status)
        *status = (s.code & 255u) << 8;
    if (pid == ssh_child_pid)
        ssh_child_pid = 0;
    return r;
}

uid_t getuid(void)
{
    return lume_syscall(LUME_UID, 0, 0, 0);
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
    return 0;
}

pid_t getpid(void)
{
    return lume_syscall(LUME_PID, 0, 0, 0);
}

struct passwd *getpwnam(const char *name)
{
    static struct passwd p;
    if (strcmp(name, "root"))
        return NULL;
    memset(&p, 0, sizeof p);
    p.pw_name = "root";
    p.pw_passwd = "*";
    p.pw_dir = "/root";
    p.pw_shell = "/usr/bin/sshshell" LUME_APP_SUFFIX "";
    return &p;
}

struct passwd *getpwuid(uid_t uid)
{
    return uid ? NULL : getpwnam("root");
}

static int shell_cursor;

void setusershell(void)
{
    shell_cursor = 0;
}

void endusershell(void)
{
    shell_cursor = 0;
}

char *getusershell(void)
{
    return shell_cursor++ ? NULL : "/usr/bin/sshshell" LUME_APP_SUFFIX "";
}

pid_t setsid(void)
{
    errno = ENOSYS;
    return -1;
}

pid_t getppid(void)
{
    uint32_t own = getpid();
    for (unsigned i = 0; i < 8; i++)
    {
        struct lume_process_info p;
        if (lume_syscall(LUME_PS, i, (uint32_t)(uintptr_t)&p, 0) > 0 && p.pid == own)
            return p.parent;
    }
    return -1;
}

int kill(pid_t pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = ENOSYS;
    return -1;
}

pid_t fork(void)
{
    errno = ENOSYS;
    return -1;
}

int getrlimit(int which, struct rlimit *r)
{
    if (which == RLIMIT_CORE)
    {
        r->rlim_cur = r->rlim_max = 0;
        return 0;
    }
    if (which == RLIMIT_NOFILE)
    {
        r->rlim_cur = r->rlim_max = 40;
        return 0;
    }
    errno = EINVAL;
    return -1;
}

int setrlimit(int which, const struct rlimit *r)
{
    if (which == RLIMIT_CORE && !r->rlim_cur)
        return 0;
    errno = ENOSYS;
    return -1;
}

int clock_gettime(clockid_t id, struct timespec *t)
{
    if (id != CLOCK_MONOTONIC)
    {
        errno = ENOSYS;
        return -1;
    }
    uint32_t ms = net_now();
    t->tv_sec = ms / 1000;
    t->tv_nsec = (ms % 1000) * 1000000;
    return 0;
}

int gettimeofday(struct timeval *t, void *zone)
{
    (void)zone;
    if (t)
    {
        t->tv_sec = 0;
        t->tv_usec = 0;
    }
    errno = ENOSYS;
    return -1;
}

ssize_t getrandom(void *p, size_t n, unsigned flags)
{
    if (flags & ~GRND_NONBLOCK)
    {
        errno = EINVAL;
        return -1;
    }
    if (n > 256)
        n = 256;
    unsigned t = ssh_profile_now();
    int r = app_crypto(LUME_CRYPTO_RANDOM, p, n);
    ssh_profile_record(SSH_P_RNG, ssh_profile_now() - t);
    return result(r);
}
static struct sigaction handlers[32];

int sigaction(int n, const struct sigaction *action, struct sigaction *old)
{
    if (n < 1 || n >= 32)
    {
        errno = EINVAL;
        return -1;
    }
    if (old)
        *old = handlers[n];
    if (action)
        handlers[n] = *action;
    return 0;
}

int lstat(const char *p, struct stat *s)
{
    return stat(p, s);
}

int gethostname(char *s, size_t n)
{
    if (n < 5)
    {
        errno = ENAMETOOLONG;
        return -1;
    }
    memcpy(s, "lume", 5);
    return 0;
}

int getgroups(int n, gid_t *groups)
{
    (void)n;
    (void)groups;
    errno = ENOSYS;
    return -1;
}

int chmod(const char *p, mode_t m)
{
    (void)p;
    (void)m;
    errno = ENOSYS;
    return -1;
}

int chown(const char *p, uid_t u, gid_t g)
{
    (void)p;
    (void)u;
    (void)g;
    errno = ENOSYS;
    return -1;
}

clock_t times(struct tms *t)
{
    (void)t;
    errno = ENOSYS;
    return (clock_t)-1;
}

int nanosleep(const struct timespec *request, struct timespec *remaining)
{
    if (!request || request->tv_sec < 0 || request->tv_nsec < 0 || request->tv_nsec >= 1000000000)
    {
        errno = EINVAL;
        return -1;
    }
    uint64_t ms = (uint64_t)request->tv_sec * 1000u + (request->tv_nsec + 999999u) / 1000000u;
    if (ms > UINT32_MAX)
    {
        errno = EINVAL;
        return -1;
    }
    (void)lume_syscall(LUME_SLEEP, (uint32_t)ms, 0, 0);
    if (remaining)
        *remaining = (struct timespec){0};
    return 0;
}
