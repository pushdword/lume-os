/* SPDX-License-Identifier: BSD-3-Clause */
#define _GNU_SOURCE
#include "wayland-os.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

int wl_os_socket_cloexec(int domain, int type, int protocol)
{
    return socket(domain, type | SOCK_CLOEXEC, protocol);
}

int wl_os_socket_peercred(int fd, uid_t *uid, gid_t *gid, pid_t *pid)
{
    if (getpeereid(fd, uid, gid))
        return -1;
    *pid = 0;
    return 0;
}

int wl_os_dupfd_cloexec(int fd, int minimum)
{
    return fcntl(fd, F_DUPFD_CLOEXEC, minimum);
}

ssize_t wl_os_recvmsg_cloexec(int fd, struct msghdr *message, int flags)
{
    return recvmsg(fd, message, flags | MSG_CMSG_CLOEXEC);
}

int wl_os_accept_cloexec(int fd, struct sockaddr *address, socklen_t *bytes)
{
    int accepted = accept(fd, address, bytes);
    if (accepted >= 0 && fcntl(accepted, F_SETFD, FD_CLOEXEC) < 0)
    {
        int saved = errno;
        close(accepted);
        errno = saved;
        return -1;
    }
    return accepted;
}

void *wl_os_mremap_maymove(int fd, void *previous, ssize_t *previous_bytes, ssize_t bytes,
                           int protection, int flags)
{
    void *mapped = mmap(NULL, bytes, protection, flags, fd, 0);
    if (mapped != MAP_FAILED && !munmap(previous, *previous_bytes))
        *previous_bytes = 0;
    return mapped;
}
