#ifndef LUME_OPENSSH_NATIVE_H
#define LUME_OPENSSH_NATIVE_H
#include "../config.h"
#include <stdint.h>
#include <errno.h>
#include "lume/abi.h"

#include "lume/syscall_native.h"
#define native_call lume_native_call

static inline int native_result(int64_t result)
{
    if (result >= 0)
        return (int)result;
    switch (-result)
    {
    case 1:
        errno = EPERM;
        break;
    case 2:
        errno = ENOENT;
        break;
    case 3:
        errno = ESRCH;
        break;
    case 4:
        errno = EINTR;
        break;
    case 5:
        errno = EIO;
        break;
    case 7:
        errno = E2BIG;
        break;
    case 8:
        errno = ENOEXEC;
        break;
    case 9:
        errno = EBADF;
        break;
    case 10:
        errno = ECHILD;
        break;
    case 11:
        errno = EAGAIN;
        break;
    case 12:
        errno = ENOMEM;
        break;
    case 13:
        errno = EACCES;
        break;
    case 14:
        errno = EFAULT;
        break;
    case 16:
        errno = EBUSY;
        break;
    case 17:
        errno = EEXIST;
        break;
    case 20:
        errno = ENOTDIR;
        break;
    case 21:
        errno = EISDIR;
        break;
    case 22:
        errno = EINVAL;
        break;
    case 24:
        errno = EMFILE;
        break;
    case 27:
        errno = EFBIG;
        break;
    case 28:
        errno = ENOSPC;
        break;
    case 29:
        errno = ESPIPE;
        break;
    case 30:
        errno = EROFS;
        break;
    case 32:
        errno = EPIPE;
        break;
    case 36:
        errno = ENAMETOOLONG;
        break;
    case 38:
        errno = ENOSYS;
        break;
    case 39:
        errno = ENOTEMPTY;
        break;
    case 98:
        errno = EADDRINUSE;
        break;
    case 104:
        errno = ECONNRESET;
        break;
    case 107:
        errno = ENOTCONN;
        break;
    case 110:
        errno = ETIMEDOUT;
        break;
    case 111:
        errno = ECONNREFUSED;
        break;
    case 115:
        errno = EINPROGRESS;
        break;
    default:
        errno = EIO;
        break;
    }
    return -1;
}

static inline int unsupported(void)
{
    errno = ENOSYS;
    return -1;
}
#endif
