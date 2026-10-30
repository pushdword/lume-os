#ifdef __cplusplus
extern "C"
{
#endif
#ifndef LUME_POSIX_SOCKET_H
#define LUME_POSIX_SOCKET_H
#include <sys/types.h>
#include <stdint.h>
#include <stddef.h>
    typedef uint32_t socklen_t;
    typedef uint16_t sa_family_t;

    struct sockaddr
    {
        sa_family_t sa_family;
        char sa_data[14];
    };

    struct sockaddr_storage
    {
        sa_family_t ss_family;
        unsigned char data[126];
    };

#define AF_UNSPEC 0
#define AF_UNIX 1
#define AF_INET 2
#define AF_INET6 10
#define PF_INET AF_INET
#define PF_UNSPEC AF_UNSPEC
#define PF_UNIX AF_UNIX
#define SOCK_STREAM 1
#define SOCK_DGRAM 2
#define SOL_SOCKET 1
#define SO_REUSEADDR 2
#define SO_TYPE 3
#define SO_ERROR 4
#define SO_KEEPALIVE 9
#define SHUT_RD 0
#define SHUT_WR 1
#define SHUT_RDWR 2
#define MSG_PEEK 2
#define MSG_NOSIGNAL 0x4000
    int socket(int, int, int);
    int bind(int, const struct sockaddr *, socklen_t);
    int listen(int, int);
    int accept(int, struct sockaddr *, socklen_t *);
    int connect(int, const struct sockaddr *, socklen_t);
    int setsockopt(int, int, int, const void *, socklen_t);
    int getsockopt(int, int, int, void *, socklen_t *);
    int getsockname(int, struct sockaddr *, socklen_t *);
    int getpeername(int, struct sockaddr *, socklen_t *);
    int shutdown(int, int);
    ssize_t recv(int, void *, size_t, int);
    ssize_t send(int, const void *, size_t, int);
    int socketpair(int, int, int, int[2]);
    struct iovec;

    struct msghdr
    {
        void *msg_name;
        socklen_t msg_namelen;
        struct iovec *msg_iov;
        size_t msg_iovlen;
        void *msg_control;
        size_t msg_controllen;
        int msg_flags;
    };

    struct cmsghdr
    {
        size_t cmsg_len;
        int cmsg_level, cmsg_type;
    };

#define CMSG_ALIGN(n) (((n) + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1))
#define CMSG_LEN(n) (CMSG_ALIGN(sizeof(struct cmsghdr)) + (n))
#define CMSG_SPACE(n) (CMSG_ALIGN(sizeof(struct cmsghdr)) + CMSG_ALIGN(n))
#define CMSG_DATA(c) ((unsigned char *)(c) + CMSG_ALIGN(sizeof(struct cmsghdr)))
#define CMSG_FIRSTHDR(m)                                                                           \
    ((m)->msg_controllen >= sizeof(struct cmsghdr) ? (struct cmsghdr *)(m)->msg_control : NULL)
#define CMSG_NXTHDR(m, c)                                                                          \
    ((unsigned char *)(c) + CMSG_ALIGN((c)->cmsg_len) + sizeof(struct cmsghdr) <=                  \
             (unsigned char *)(m)->msg_control + (m)->msg_controllen                               \
         ? (struct cmsghdr *)((unsigned char *)(c) + CMSG_ALIGN((c)->cmsg_len))                    \
         : NULL)
#define SCM_RIGHTS 1
#define MSG_CTRUNC 8
#define MSG_TRUNC 32
#define SOCK_CLOEXEC 0x80000
#define SOCK_NONBLOCK 0x800
#define SO_RCVBUF 8
#define SO_SNDBUF 7
#define SO_RCVTIMEO 20
#define SO_SNDTIMEO 21
#define SO_OOBINLINE 10
    ssize_t sendmsg(int, const struct msghdr *, int);
    ssize_t recvmsg(int, struct msghdr *, int);
#endif
#ifdef __cplusplus
}
#endif
