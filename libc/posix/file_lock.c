#include "lume/abi.h"
#include "lume/file_io.h"
#include "lume/file_lock.h"
#include "errno_map.h"
#ifndef LUME_USER_HOST_TEST
#include "lume/syscall_native.h"
#else
extern int64_t lume_abi_host_call(unsigned, uint64_t, uint64_t, uint64_t);
#define lume_native_call lume_abi_host_call
#endif
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <unistd.h>
#include <stdbool.h>

static int lock_call(int fd, unsigned operation, struct flock *lock, bool ofd)
{
    bool query = operation == (ofd ? F_OFD_GETLK : F_GETLK);
    if (!query && operation != (ofd ? F_OFD_SETLK : F_SETLK))
    {
        errno = EINVAL;
        return -1;
    }
    if (!lock)
    {
        errno = EFAULT;
        return -1;
    }
    unsigned type = lock->l_type == F_RDLCK   ? LUME_FILE_LOCK_READ
                    : lock->l_type == F_WRLCK ? LUME_FILE_LOCK_WRITE
                    : lock->l_type == F_UNLCK ? LUME_FILE_LOCK_UNLOCK
                                              : 0;
    if (!type || (ofd && lock->l_pid) || lock->l_whence < SEEK_SET || lock->l_whence > SEEK_END ||
        (query && type == LUME_FILE_LOCK_UNLOCK))
    {
        errno = EINVAL;
        return -1;
    }
    struct lume_file_lock_request request = {.version = LUME_FILE_LOCK_VERSION,
                                             .bytes = sizeof(request),
                                             .fd = fd,
                                             .type = type,
                                             .whence = lock->l_whence,
                                             .start = lock->l_start,
                                             .length = lock->l_len};
    unsigned native_operation = ofd ? (query ? LUME_FILE_IO_OFD_GETLK : LUME_FILE_IO_OFD_SETLK)
                                    : (query ? LUME_FILE_IO_POSIX_GETLK : LUME_FILE_IO_POSIX_SETLK);
    int64_t result =
        lume_native_call(LUME_SYS_FILE_IO, native_operation, (uintptr_t)&request, sizeof(request));
    if (result < 0)
    {
        errno = lume_errno_from_syscall((unsigned)-result);
        return -1;
    }
    if (result)
    {
        errno = EIO;
        return -1;
    }
    if (query)
    {
        if (request.type < LUME_FILE_LOCK_READ || request.type > LUME_FILE_LOCK_UNLOCK)
        {
            errno = EIO;
            return -1;
        }
        if (request.type != LUME_FILE_LOCK_UNLOCK)
        {
            __typeof__(lock->l_start) start = request.start;
            __typeof__(lock->l_len) length = request.length;
            __typeof__(lock->l_pid) pid = request.pid;
            if ((int64_t)start != request.start || (int64_t)length != request.length ||
                (int64_t)pid != request.pid)
            {
                errno = EOVERFLOW;
                return -1;
            }
            lock->l_whence = SEEK_SET;
            lock->l_start = start;
            lock->l_len = length;
            lock->l_pid = pid;
        }
        lock->l_type = request.type == LUME_FILE_LOCK_READ    ? F_RDLCK
                       : request.type == LUME_FILE_LOCK_WRITE ? F_WRLCK
                                                              : F_UNLCK;
    }
    return 0;
}

int lume_posix_ofd_lock(int fd, unsigned operation, struct flock *lock)
{
    return lock_call(fd, operation, lock, true);
}

int lume_posix_record_lock(int fd, unsigned operation, struct flock *lock)
{
    return lock_call(fd, operation, lock, false);
}
