#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

typedef int64_t positioned_off_t;
#define off_t positioned_off_t
#define pread test_pread
#define pwrite test_pwrite
#define ftruncate test_ftruncate
#define LUME_USER_HOST_TEST 1
#ifndef LUME_TEST_POSITIONED_IO_SOURCE
#define LUME_TEST_POSITIONED_IO_SOURCE "../../libc/posix/positioned_io.c"
#endif
#include LUME_TEST_POSITIONED_IO_SOURCE

static struct lume_file_io_request seen;
static unsigned operation_seen, calls;
static int64_t syscall_result;

int lume_errno_from_syscall(unsigned error)
{
    return error == LUME_EBADF    ? EBADF
           : error == LUME_ESPIPE ? ESPIPE
           : error == LUME_ENOSYS ? ENOSYS
                                  : EIO;
}

int64_t lume_abi_host_call(unsigned number, uint64_t operation, uint64_t address, uint64_t bytes)
{
    assert(number == LUME_SYS_FILE_IO && bytes == sizeof(seen));
    seen = *(const struct lume_file_io_request *)(uintptr_t)address;
    assert(seen.version == LUME_FILE_IO_VERSION && seen.bytes == sizeof(seen) && !seen.whence);
    operation_seen = (unsigned)operation;
    ++calls;
    return syscall_result;
}

int main(void)
{
    void *buffer = (void *)(uintptr_t)UINT64_C(0x100000083);
    const int64_t high = INT64_C(0x100020003);
    syscall_result = 7;
    errno = EBUSY;
    assert(test_pread(9, buffer, 11, high) == 7 && errno == EBUSY);
    assert(seen.fd == 9 && seen.buffer == (uintptr_t)buffer && seen.count == 11 &&
           seen.offset == high && operation_seen == LUME_FILE_IO_PREAD);
    assert(test_pwrite(13, buffer, LUME_FILE_IO_MAX + 17, high) == 7);
    assert(seen.fd == 13 && seen.count == LUME_FILE_IO_MAX &&
           operation_seen == LUME_FILE_IO_PWRITE);
    syscall_result = -LUME_EBADF;
    assert(test_pread(-1, buffer, 1, 0) == -1 && errno == EBADF);
    syscall_result = -LUME_ESPIPE;
    assert(test_pwrite(9, buffer, 1, 0) == -1 && errno == ESPIPE);
    unsigned before = calls;
    assert(test_pread(9, buffer, 1, -1) == -1 && errno == EINVAL);
    assert(test_pwrite(9, buffer, 17, INT64_MAX) == -1 && errno == EINVAL);
    assert(test_pread(9, buffer, SIZE_MAX, 0) == -1 && errno == EINVAL && calls == before);
    syscall_result = 0;
    assert(test_pread(9, NULL, 0, high) == 0 && !seen.count && !seen.buffer);
    syscall_result = 2;
    assert(test_pread(9, buffer, 1, high) == -1 && errno == EIO);
    syscall_result = 0;
    errno = EBUSY;
    assert(test_ftruncate(17, high) == 0 && errno == EBUSY);
    assert(operation_seen == LUME_FILE_IO_TRUNCATE && seen.fd == 17 && seen.offset == high &&
           !seen.count && !seen.buffer && !seen.whence);
    syscall_result = -LUME_ENOSYS;
    before = calls;
    assert(test_ftruncate(17, 33) == -1 && errno == ENOSYS && calls == before + 1);
    before = calls;
    assert(test_ftruncate(17, -1) == -1 && errno == EINVAL && calls == before);
    syscall_result = 1;
    assert(test_ftruncate(17, 0) == -1 && errno == EIO);
    puts("PASS positional libc requests, 64-bit pointers and offsets, short transfers and errno");
    return 0;
}
