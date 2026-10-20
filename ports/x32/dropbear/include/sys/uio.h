#ifndef LUME_SSH_UIO_H
#define LUME_SSH_UIO_H
#include <sys/types.h>
struct iovec {void *iov_base;size_t iov_len;};
ssize_t writev(int,const struct iovec *,int);
#endif
