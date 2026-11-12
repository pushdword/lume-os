/* SPDX-License-Identifier: BSD-3-Clause */
#include "lume/render_native.h"
#include "lume/abi.h"
#include "lume/syscall_native.h"
#include "../posix/errno_map.h"
#include <errno.h>

int lume_render_control(unsigned action, struct lume_render_request *request)
{
    if (!request)
    {
        errno = EFAULT;
        return -1;
    }
    request->version = LUME_RENDER_VERSION;
    request->bytes = sizeof(*request);
    int64_t result = lume_native_call(LUME_SYS_GPU, action, (uintptr_t)request, sizeof(*request));
    if (result < 0)
    {
        errno = lume_errno_from_syscall((unsigned)-result);
        return -1;
    }
    return 0;
}

int lume_render_open(unsigned flags)
{
    struct lume_render_request request = {.fd = -1, .flags = flags};
    return lume_render_control(LUME_RENDER_OPEN, &request) ? -1 : request.fd;
}

int lume_render_query(int fd, struct lume_render_info *info)
{
    struct lume_render_request request = {
        .fd = fd, .data = (uintptr_t)info, .length = sizeof(*info)};
    return lume_render_control(LUME_RENDER_INFO, &request);
}

int lume_render_pci(int fd, struct lume_render_pci *info)
{
    struct lume_render_request request = {
        .fd = fd, .data = (uintptr_t)info, .length = sizeof(*info)};
    return lume_render_control(LUME_RENDER_PCI, &request);
}

int lume_render_display(int fd, unsigned index, struct lume_render_display *info)
{
    struct lume_render_request request = {
        .fd = fd, .index = index, .data = (uintptr_t)info, .length = sizeof(*info)};
    return lume_render_control(LUME_RENDER_DISPLAY, &request);
}

int lume_render_present(int fd, const struct lume_render_present *present)
{
    struct lume_render_request request = {
        .fd = fd, .data = (uintptr_t)present, .length = sizeof(*present)};
    return lume_render_control(LUME_RENDER_PRESENT, &request);
}

int lume_render_context(int fd, unsigned index, unsigned id)
{
    struct lume_render_request request = {.fd = fd, .id = id, .index = index};
    return lume_render_control(LUME_RENDER_CONTEXT, &request);
}

int lume_render_capability_info(int fd, unsigned index, unsigned *id, unsigned *revision,
                                size_t *bytes)
{
    if (!id || !revision || !bytes)
    {
        errno = EFAULT;
        return -1;
    }
    struct lume_render_request request = {.fd = fd, .index = index};
    if (lume_render_control(LUME_RENDER_CAPSET_INFO, &request))
        return -1;
    if (request.length > SIZE_MAX)
    {
        errno = EOVERFLOW;
        return -1;
    }
    *id = request.id;
    *revision = request.revision;
    *bytes = (size_t)request.length;
    return 0;
}

int lume_render_capability(int fd, unsigned index, unsigned id, unsigned revision, void *data,
                           size_t bytes)
{
    struct lume_render_request request = {.fd = fd,
                                          .index = index,
                                          .id = id,
                                          .revision = revision,
                                          .data = (uintptr_t)data,
                                          .length = bytes};
    return lume_render_control(LUME_RENDER_CAPSET, &request);
}

int lume_render_resource_create(int fd, struct lume_render_resource *resource)
{
    struct lume_render_request request = {
        .fd = fd, .data = (uintptr_t)resource, .length = sizeof(*resource)};
    return lume_render_control(LUME_RENDER_RESOURCE_CREATE, &request);
}

int lume_render_resource_export(int fd, unsigned handle)
{
    struct lume_render_request request = {.fd = fd, .id = handle};
    return lume_render_control(LUME_RENDER_RESOURCE_EXPORT, &request) ? -1 : (int)request.value;
}

int lume_render_resource_close(int fd, unsigned handle)
{
    struct lume_render_request request = {.fd = fd, .id = handle};
    return lume_render_control(LUME_RENDER_RESOURCE_CLOSE, &request);
}

int lume_render_resource_info(int fd, unsigned handle, struct lume_render_resource *resource)
{
    struct lume_render_request request = {
        .fd = fd, .id = handle, .data = (uintptr_t)resource, .length = sizeof(*resource)};
    return lume_render_control(LUME_RENDER_RESOURCE_INFO, &request);
}

int lume_render_resource_wait(int fd, unsigned handle, unsigned milliseconds)
{
    struct lume_render_request request = {.fd = fd, .id = handle, .length = milliseconds};
    return lume_render_control(LUME_RENDER_RESOURCE_WAIT, &request);
}

int lume_render_prime_export(int fd, unsigned handle, unsigned flags)
{
    struct lume_render_request request = {.fd = fd, .id = handle, .flags = flags};
    return lume_render_control(LUME_RENDER_PRIME_EXPORT, &request) ? -1 : (int)request.value;
}

int lume_render_prime_import(int fd, int prime, unsigned *handle)
{
    if (!handle)
    {
        errno = EFAULT;
        return -1;
    }
    struct lume_render_request request = {.fd = fd, .value = (unsigned)prime};
    if (lume_render_control(LUME_RENDER_PRIME_IMPORT, &request))
        return -1;
    *handle = request.id;
    return 0;
}

int lume_render_transfer(int fd, unsigned flags, const struct lume_render_transfer *transfer)
{
    struct lume_render_request request = {
        .fd = fd, .flags = flags, .data = (uintptr_t)transfer, .length = sizeof(*transfer)};
    return lume_render_control(LUME_RENDER_TRANSFER, &request);
}

int lume_render_submit(int fd, const void *data, size_t bytes, uint64_t *fence)
{
    return lume_render_submit_resources(fd, data, bytes, NULL, 0, fence);
}

int lume_render_submit_resources(int fd, const void *data, size_t bytes, const uint32_t *handles,
                                 unsigned count, uint64_t *fence)
{
    if (!fence)
    {
        errno = EFAULT;
        return -1;
    }
    struct lume_render_request request = {.fd = fd, .data = (uintptr_t)data, .length = bytes};
    request.index = count;
    request.value = count ? (uintptr_t)handles : 0;
    if (lume_render_control(LUME_RENDER_SUBMIT, &request))
        return -1;
    *fence = request.value;
    return 0;
}

int lume_render_wait(int fd, uint64_t fence, unsigned milliseconds)
{
    struct lume_render_request request = {.fd = fd, .value = fence, .length = milliseconds};
    return lume_render_control(LUME_RENDER_WAIT, &request);
}
