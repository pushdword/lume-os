/* Lume ABI0 uses Linux errno numbers; Picolibc uses Newlib's numbering.
 * List the kernel contract explicitly, including values which happen to agree.
 * Unknown ABI errors become EIO rather than an unrelated Picolibc errno.
 */
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
    case 95:
        return EOPNOTSUPP;
    case 110:
        return ETIMEDOUT;
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
    return mapped(app_seek(f, p, w));
}

int lume_posix_sync(int f)
{
    return mapped(app_sync(f));
}

int lume_posix_stat(const char *p, struct lume_stat *s)
{
    return mapped(app_stat(p, s));
}
