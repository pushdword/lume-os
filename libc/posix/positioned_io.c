#include "lume/abi.h"
#include "lume/file_io.h"
#include "errno_map.h"
#ifndef LUME_USER_HOST_TEST
#include "lume/syscall_native.h"
#else
extern int64_t lume_abi_host_call(unsigned, uint64_t, uint64_t, uint64_t);
#define lume_native_call lume_abi_host_call
#endif
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

static ssize_t positioned_io(int fd, void *buffer, size_t count, off_t offset, unsigned operation)
{
    if (offset < 0 || count > INT64_MAX || count > INT64_MAX - (uint64_t)offset)
    {
        errno = EINVAL;
        return -1;
    }
    struct lume_file_io_request request = {.version = LUME_FILE_IO_VERSION,
                                           .bytes = sizeof(request),
                                           .fd = fd,
                                           .buffer = (uintptr_t)buffer,
                                           .count =
                                               count > LUME_FILE_IO_MAX ? LUME_FILE_IO_MAX : count,
                                           .offset = offset};
    int64_t result =
        lume_native_call(LUME_SYS_FILE_IO, operation, (uintptr_t)&request, sizeof(request));
    if (result < 0)
    {
        errno = lume_errno_from_syscall((unsigned)-result);
        return -1;
    }
    if ((uint64_t)result > request.count)
    {
        errno = EIO;
        return -1;
    }
    return (ssize_t)result;
}

ssize_t pread(int fd, void *buffer, size_t count, off_t offset)
{
    return positioned_io(fd, buffer, count, offset, LUME_FILE_IO_PREAD);
}

ssize_t pwrite(int fd, const void *buffer, size_t count, off_t offset)
{
    return positioned_io(fd, (void *)buffer, count, offset, LUME_FILE_IO_PWRITE);
}

int ftruncate(int fd, off_t length)
{
    if (length < 0)
    {
        errno = EINVAL;
        return -1;
    }
    struct lume_file_io_request request = {
        .version = LUME_FILE_IO_VERSION, .bytes = sizeof(request), .fd = fd, .offset = length};
    int64_t result = lume_native_call(LUME_SYS_FILE_IO, LUME_FILE_IO_TRUNCATE, (uintptr_t)&request,
                                      sizeof(request));
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
    return 0;
}
