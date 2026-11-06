/* Lume ABI0 uses Linux errno numbers. Picolibc uses Newlib errno numbers. Map the kernel values
 * explicitly, including matching values. Unknown ABI errors map to EIO. */
#include "errno_map.h"
#include "api.h"
#include <errno.h>

int lume_errno_from_syscall(unsigned error)
{
    switch (error)
    {
    case 0:
        return 0;
    case 1:
        return EPERM;
    case 2:
        return ENOENT;
    case 3:
        return ESRCH;
    case 4:
        return EINTR;
    case 5:
        return EIO;
    case 6:
        return ENXIO;
    case 7:
        return E2BIG;
    case 8:
        return ENOEXEC;
    case 9:
        return EBADF;
    case 10:
        return ECHILD;
    case 11:
        return EAGAIN;
    case 12:
        return ENOMEM;
    case 13:
        return EACCES;
    case 14:
        return EFAULT;
    case 15:
        return ENOTBLK;
    case 16:
        return EBUSY;
    case 17:
        return EEXIST;
    case 18:
        return EXDEV;
    case 19:
        return ENODEV;
    case 20:
        return ENOTDIR;
    case 21:
        return EISDIR;
    case 22:
        return EINVAL;
    case 23:
        return ENFILE;
    case 24:
        return EMFILE;
    case 25:
        return ENOTTY;
    case 26:
        return ETXTBSY;
    case 27:
        return EFBIG;
    case 28:
        return ENOSPC;
    case 29:
        return ESPIPE;
    case 30:
        return EROFS;
    case 31:
        return EMLINK;
    case 32:
        return EPIPE;
    case 33:
        return EDOM;
    case 34:
        return ERANGE;
    case 35:
        return EDEADLK;
    case 36:
        return ENAMETOOLONG;
    case 37:
        return ENOLCK;
    case 38:
        return ENOSYS;
    case 39:
        return ENOTEMPTY;
    case 40:
        return ELOOP;
    case 75:
        return EOVERFLOW;
    case 88:
        return ENOTSOCK;
    case 89:
        return EDESTADDRREQ;
    case 90:
        return EMSGSIZE;
    case 91:
        return EPROTOTYPE;
    case 92:
        return ENOPROTOOPT;
    case 93:
        return EPROTONOSUPPORT;
    case 94:
        return ESOCKTNOSUPPORT;
    case 95:
        return EOPNOTSUPP;
    case 96:
        return EPFNOSUPPORT;
    case 97:
        return EAFNOSUPPORT;
    case 98:
        return EADDRINUSE;
    case 99:
        return EADDRNOTAVAIL;
    case 100:
        return ENETDOWN;
    case 101:
        return ENETUNREACH;
    case 102:
        return ENETRESET;
    case 103:
        return ECONNABORTED;
    case 104:
        return ECONNRESET;
    case 105:
        return ENOBUFS;
    case 106:
        return EISCONN;
    case 107:
        return ENOTCONN;
    case 108:
        return ESHUTDOWN;
    case 109:
        return ETOOMANYREFS;
    case 110:
        return ETIMEDOUT;
    case 111:
        return ECONNREFUSED;
    case 112:
        return EHOSTDOWN;
    case 113:
        return EHOSTUNREACH;
    case 114:
        return EALREADY;
    case 115:
        return EINPROGRESS;
    case 116:
        return ESTALE;
    case 125:
        return ECANCELED;
    default:
        return EIO;
    }
}

static int32_t mapped(int32_t result)
{
    return result < 0 ? -lume_errno_from_syscall((unsigned)-result) : result;
}

int32_t lume_posix_syscall(uint32_t n, uint32_t a, uint32_t b, uint32_t c)
{
    return mapped(lume_syscall(n, a, b, c));
}

int32_t lume_posix_app_call(uint32_t n, uintptr_t a, uintptr_t b, uintptr_t c)
{
    return mapped(lume_app_call(n, a, b, c));
}

int lume_posix_open(const char *p, unsigned f)
{
    return mapped(app_open(p, f));
}

int lume_posix_close(int f)
{
    return mapped(app_close(f));
}

int lume_posix_read(int f, void *p, unsigned n)
{
    return mapped(app_read(f, p, n));
}

int lume_posix_write(int f, const void *p, unsigned n)
{
    return mapped(app_write(f, p, n));
}

int lume_posix_seek(int f, int32_t p, unsigned w)
{
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64)
    extern int32_t lume_aarch64_posix_seek(int, int32_t, unsigned);
    return mapped(lume_aarch64_posix_seek(f, p, w));
#else
    return mapped(app_seek(f, p, w));
#endif
}

int lume_posix_sync(int f)
{
    return mapped(app_sync(f));
}

int lume_posix_stat(const char *p, struct lume_stat *s)
{
    return mapped(app_stat(p, s));
}
