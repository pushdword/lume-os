#ifndef LUME_SSH_SOCKET_H
#define LUME_SSH_SOCKET_H
#include <sys/types.h>
#include <stdint.h>
typedef uint32_t socklen_t;
typedef uint16_t sa_family_t;
struct sockaddr {sa_family_t sa_family;char sa_data[14];};
struct sockaddr_storage {sa_family_t ss_family;unsigned char data[126];};
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
int socket(int,int,int);int bind(int,const struct sockaddr *,socklen_t);
int listen(int,int);int accept(int,struct sockaddr *,socklen_t *);
int connect(int,const struct sockaddr *,socklen_t);
int setsockopt(int,int,int,const void *,socklen_t);
int getsockopt(int,int,int,void *,socklen_t *);
int getsockname(int,struct sockaddr *,socklen_t *);
int getpeername(int,struct sockaddr *,socklen_t *);
int shutdown(int,int);
ssize_t recv(int,void *,size_t,int);ssize_t send(int,const void *,size_t,int);
int socketpair(int,int,int,int [2]);
#endif
