#include "lume/abi.h"
#include "lume/memory.h"
#include "lume/syscall_native.h"
#include "errno_map.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

int64_t lume_memory_call(unsigned operation, struct lume_vm_request *request)
{
    request->version = LUME_VM_VERSION;
    int64_t result = lume_native_call(LUME_SYS_VM, operation, (uintptr_t)request, sizeof(*request));
    return result < 0 ? -(int64_t)lume_errno_from_syscall((unsigned)-result) : result;
}

static int result(int64_t value)
{
    if (value >= 0)
        return (int)value;
    errno = (int)-value;
    return -1;
}

static int jit_scope(unsigned operation)
{
    int64_t value = lume_native_call(LUME_SYS_VM, operation, 0, 0);
    return result(value < 0 ? -(int64_t)lume_errno_from_syscall((unsigned)-value) : value);
}

int lume_jit_write_begin(void)
{
    return jit_scope(LUME_VM_JIT_BEGIN);
}

int lume_jit_write_end(void)
{
    return jit_scope(LUME_VM_JIT_END);
}

void *lume_mmap_aligned(void *address, size_t bytes, int protection, int flags, int fd,
                        off_t offset, size_t alignment)
{
    if (offset < 0)
    {
        errno = EINVAL;
        return MAP_FAILED;
    }
    struct lume_vm_request request = {.address = (uintptr_t)address,
                                      .length = bytes,
                                      .protection = (unsigned)protection,
                                      .flags = (unsigned)flags,
                                      .fd = fd,
                                      .offset = (uint64_t)offset,
                                      .alignment = alignment};
    int64_t value = lume_memory_call(LUME_VM_MAP, &request);
    if (value < 0)
    {
        errno = (int)-value;
        return MAP_FAILED;
    }
    return (void *)(uintptr_t)value;
}

void *mmap(void *address, size_t bytes, int protection, int flags, int fd, off_t offset)
{
    return lume_mmap_aligned(address, bytes, protection, flags, fd, offset, 4096);
}

int munmap(void *address, size_t bytes)
{
    struct lume_vm_request request = {.address = (uintptr_t)address, .length = bytes};
    return result(lume_memory_call(LUME_VM_UNMAP, &request));
}

int mprotect(void *address, size_t bytes, int protection)
{
    struct lume_vm_request request = {
        .address = (uintptr_t)address, .length = bytes, .protection = (unsigned)protection};
    return result(lume_memory_call(LUME_VM_PROTECT, &request));
}

int lume_clear_cache(void *address, size_t bytes)
{
    struct lume_vm_request request = {.address = (uintptr_t)address, .length = bytes};
    return result(lume_memory_call(LUME_VM_SYNC, &request));
}

int madvise(void *address, size_t bytes, int advice)
{
    struct lume_vm_request request = {
        .address = (uintptr_t)address, .length = bytes, .advice = (unsigned)advice};
    return result(lume_memory_call(LUME_VM_ADVISE, &request));
}

int posix_madvise(void *address, size_t bytes, int advice)
{
    int saved = errno;
    int error = madvise(address, bytes, advice) ? errno : 0;
    errno = saved;
    return error;
}

static int named(unsigned operation, const char *name, unsigned flags, unsigned mode)
{
    struct lume_vm_request request = {.flags = flags, .advice = mode};
    if (!name)
    {
        errno = EFAULT;
        return -1;
    }
    size_t bytes = strnlen(name, sizeof(request.name));
    if (bytes == sizeof(request.name))
    {
        errno = ENAMETOOLONG;
        return -1;
    }
    memcpy(request.name, name, bytes + 1);
    return result(lume_memory_call(operation, &request));
}

int memfd_create(const char *name, unsigned flags)
{
    return named(LUME_VM_CREATE, name, flags, 0600);
}

int shm_open(const char *name, int flags, mode_t mode)
{
    unsigned native = flags & O_ACCMODE;
    if (flags & O_CREAT)
        native |= LUME_O_CREAT;
    if (flags & O_EXCL)
        native |= LUME_O_EXCL;
    if (flags & O_TRUNC)
        native |= LUME_O_TRUNC;
    if (flags & ~(O_ACCMODE | O_CREAT | O_EXCL | O_TRUNC | O_CLOEXEC))
    {
        errno = EINVAL;
        return -1;
    }
    return named(LUME_VM_OPEN, name, native, (unsigned)mode);
}

int shm_unlink(const char *name)
{
    return named(LUME_VM_UNLINK, name, 0, 0);
}

int lume_shm_duplicate(int fd, int protection, unsigned flags)
{
    struct lume_vm_request request = {.fd = fd, .protection = (unsigned)protection, .flags = flags};
    return result(lume_memory_call(LUME_VM_DUPLICATE, &request));
}

int mlock(const void *address, size_t bytes)
{
    (void)address;
    (void)bytes;
    errno = ENOSYS;
    return -1;
}

int munlock(const void *address, size_t bytes)
{
    (void)address;
    (void)bytes;
    errno = ENOSYS;
    return -1;
}
