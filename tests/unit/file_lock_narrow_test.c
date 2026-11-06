#include "lume/file_lock.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int lume_posix_ofd_lock(int, unsigned, struct flock *);

int lume_errno_from_syscall(unsigned error)
{
    (void)error;
    return EIO;
}

int64_t lume_abi_host_call(unsigned number, uint64_t operation, uint64_t address, uint64_t bytes)
{
    (void)operation;
    assert(number == 59 && bytes == sizeof(struct lume_file_lock_request));
    struct lume_file_lock_request *request = (void *)(uintptr_t)address;
    request->type = LUME_FILE_LOCK_WRITE;
    request->start = INT64_C(0x100000003);
    request->length = 17;
    request->pid = -1;
    return 0;
}

int main(void)
{
    _Static_assert(sizeof(((struct flock *)0)->l_start) == 4, "narrow lock fields required");
    struct flock lock = {.l_type = F_WRLCK, .l_whence = SEEK_SET, .l_start = 3, .l_len = 1};
    unsigned char before[sizeof(lock)];
    memcpy(before, &lock, sizeof(lock));
    assert(lume_posix_ofd_lock(9, F_OFD_GETLK, &lock) == -1 && errno == EOVERFLOW);
    assert(!memcmp(&lock, before, sizeof(lock)));
    puts("PASS narrow flock output rejects an unrepresentable conflict without truncation");
    return 0;
}
