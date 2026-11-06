#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#define fcntl test_fcntl
#ifndef LUME_TEST_FD_SOURCE
#define LUME_TEST_FD_SOURCE "../../libc/posix/fd.c"
#endif
#include LUME_TEST_FD_SOURCE

#include "lume/file_lock.h"
#include "lume/file_io.h"

static unsigned calls, operation_seen;
static struct lume_file_lock_request seen;
static int64_t syscall_result;
static bool conflict;
static unsigned memory_calls;
#ifdef F_ADD_SEALS
static unsigned memory_operation = UINT32_MAX, memory_flags;
static int64_t memory_result;
#endif

struct lume_vm_request;

int64_t lume_memory_call(unsigned operation, struct lume_vm_request *request)
{
#ifdef F_ADD_SEALS
    assert(operation == memory_operation && request && request->fd == 13 &&
           request->flags == memory_flags);
    ++memory_calls;
    return memory_result;
#else
    (void)operation;
    (void)request;
    assert(0);
    return -EIO;
#endif
}

int lume_errno_from_syscall(unsigned error)
{
    return error == 11 ? EAGAIN : error == 37 ? ENOLCK : EIO;
}

int32_t lume_syscall(uint32_t number, uint32_t a, uint32_t b, uint32_t c)
{
    (void)number;
    (void)a;
    (void)b;
    (void)c;
    assert(0);
    return -1;
}

int64_t lume_abi_host_call(unsigned number, uint64_t operation, uint64_t address, uint64_t bytes)
{
    assert(number == 59 && bytes == sizeof(struct lume_file_lock_request));
    struct lume_file_lock_request *request = (void *)(uintptr_t)address;
    assert(request->version == LUME_FILE_LOCK_VERSION && request->bytes == sizeof(*request));
    seen = *request;
    operation_seen = (unsigned)operation;
    ++calls;
    if (!syscall_result && operation == LUME_FILE_IO_OFD_GETLK)
    {
        request->type = conflict ? LUME_FILE_LOCK_WRITE : LUME_FILE_LOCK_UNLOCK;
        request->start = INT64_C(0x100002030);
        request->length = 19;
        request->pid = conflict ? -1 : 0;
    }
    return syscall_result;
}

int main(void)
{
    struct flock lock = {
        .l_type = F_WRLCK, .l_whence = SEEK_SET, .l_start = INT64_C(0x100002033), .l_len = 3};
    errno = EBUSY;
    assert(test_fcntl(9, F_OFD_SETLK, &lock) == 0 && errno == EBUSY);
    assert(operation_seen == LUME_FILE_IO_OFD_SETLK && seen.fd == 9 && seen.start == lock.l_start &&
           seen.length == 3 && !seen.pid);
    conflict = true;
    assert(test_fcntl(11, F_OFD_GETLK, &lock) == 0);
    assert(operation_seen == LUME_FILE_IO_OFD_GETLK && seen.fd == 11);
    assert(lock.l_type == F_WRLCK && lock.l_start == INT64_C(0x100002030) && lock.l_len == 19 &&
           lock.l_pid == -1 && lock.l_whence == SEEK_SET);
    unsigned before = calls;
    assert(test_fcntl(9, F_OFD_SETLK, &lock) == -1 && errno == EINVAL && calls == before);
    lock.l_pid = 0;
    conflict = false;
    assert(test_fcntl(11, F_OFD_GETLK, &lock) == 0 && lock.l_type == F_UNLCK);
    lock.l_type = F_RDLCK;
    syscall_result = -11;
    assert(test_fcntl(9, F_OFD_SETLK, &lock) == -1 && errno == EAGAIN);
    syscall_result = -37;
    assert(test_fcntl(9, F_OFD_SETLK, &lock) == -1 && errno == ENOLCK);
    before = calls;
    assert(test_fcntl(9, F_GETLK, &lock) == -1 && errno == ENOTSUP && calls == before);
    assert(test_fcntl(9, F_SETLK, &lock) == -1 && errno == ENOTSUP && calls == before);
    assert(test_fcntl(9, F_SETLKW, &lock) == -1 && errno == ENOTSUP && calls == before);
    assert(test_fcntl(9, 38, &lock) == -1 && errno == ENOTSUP && calls == before);
    assert(test_fcntl(9, F_OFD_GETLK, NULL) == -1 && errno == EFAULT && calls == before);
    lock.l_type = F_UNLCK;
    assert(test_fcntl(9, F_OFD_GETLK, &lock) == -1 && errno == EINVAL && calls == before);
    lock = (struct flock){.l_type = F_RDLCK, .l_whence = SEEK_END, .l_start = -18, .l_len = -4};
    syscall_result = 0;
    assert(test_fcntl(9, F_OFD_SETLK, &lock) == 0);
    assert(seen.start == -18 && seen.length == -4 && seen.whence == SEEK_END);
    assert(memory_calls == 0);
#ifdef F_ADD_SEALS
    before = calls;
    memory_operation = LUME_VM_ADD_SEALS;
    memory_flags = F_SEAL_GROW | F_SEAL_WRITE;
    memory_result = 0;
    assert(test_fcntl(13, F_ADD_SEALS, (int)memory_flags) == 0);
    assert(memory_calls == 1 && calls == before);
    memory_operation = LUME_VM_GET_SEALS;
    memory_flags = 0;
    memory_result = F_SEAL_GROW;
    assert(test_fcntl(13, F_GET_SEALS) == F_SEAL_GROW);
    assert(memory_calls == 2 && calls == before);
    memory_result = -EPERM;
    assert(test_fcntl(13, F_GET_SEALS) == -1 && errno == EPERM);
    assert(memory_calls == 3 && calls == before);
#endif
#ifdef F_ADD_SEALS
    puts("PASS fcntl pointer arguments, OFD requests, seal dispatch, errors and unsupported modes");
#else
    puts(
        "PASS fcntl pointer arguments, LP64 OFD requests, query output, errors and unsupported modes");
#endif
    return 0;
}
