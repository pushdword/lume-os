/* SPDX-License-Identifier: BSD-3-Clause */
#include "config.h"
#include "xshmfenceint.h"
#include "lume/process.h"
#include <errno.h>

int xshmfence_alloc_shm(void)
{
    struct lume_system_features features;
    int64_t result = lume_native_call(LUME_SYS_FEATURES, (uintptr_t)&features, sizeof(features),
                                      LUME_FEATURE_VERSION);
    if (result < 0)
    {
        errno = lume_errno_from_syscall((unsigned)-result);
        return -1;
    }
    if (!(features.features & LUME_FEATURE_SHARED_WORD_WAIT))
    {
        errno = ENOSYS;
        return -1;
    }
    int fd = memfd_create("xshmfence", MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (fd < 0)
        return -1;
    if (ftruncate(fd, sizeof(struct xshmfence)))
    {
        int error = errno;
        close(fd);
        errno = error;
        return -1;
    }
    return fd;
}

struct xshmfence *xshmfence_map_shm(int fd)
{
    void *mapping = mmap(NULL, sizeof(struct xshmfence), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapping == MAP_FAILED)
        return NULL;
    return mapping;
}

void xshmfence_unmap_shm(struct xshmfence *fence)
{
    (void)munmap(fence, sizeof(*fence));
}
