/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_LIBDRM_VIRTGPU_H
#define LUME_LIBDRM_VIRTGPU_H
#include "lume/render_native.h"
#include "virtgpu_drm.h"

static int lume_drm_error(int error)
{
    errno = error;
    return -1;
}

static size_t lume_drm_string(char *target, size_t capacity, const char *source)
{
    size_t bytes = strlen(source);
    if (target && capacity)
        memcpy(target, source, capacity < bytes ? capacity : bytes);
    return bytes;
}

static int lume_drm_capset(int fd, unsigned wanted, unsigned *index, unsigned *revision,
                           size_t *bytes)
{
    struct lume_render_info info;
    if (lume_render_query(fd, &info))
        return -1;
    if (info.backend != LUME_RENDER_BACKEND_VIRGL)
        return lume_drm_error(EOPNOTSUPP);
    for (unsigned i = 0; i < info.capsets; ++i)
    {
        unsigned id, version;
        size_t size;
        if (lume_render_capability_info(fd, i, &id, &version, &size))
            return -1;
        if (id == wanted)
        {
            *index = i;
            *revision = version;
            *bytes = size;
            return 0;
        }
    }
    return lume_drm_error(EINVAL);
}

static int lume_drm_parameter(int fd, struct drm_virtgpu_getparam *parameter)
{
    if (!parameter || !parameter->value || parameter->value > UINTPTR_MAX)
        return lume_drm_error(EINVAL);
    struct lume_render_info info;
    if (lume_render_query(fd, &info))
        return -1;
    uint64_t value = 0;
    switch (parameter->param)
    {
    case VIRTGPU_PARAM_3D_FEATURES:
        value = info.backend == LUME_RENDER_BACKEND_VIRGL &&
                (info.operations & LUME_RENDER_OPERATION(LUME_RENDER_SUBMIT)) != 0;
        break;
    case VIRTGPU_PARAM_CAPSET_QUERY_FIX:
    case VIRTGPU_PARAM_CONTEXT_INIT:
        value = 1;
        break;
    case VIRTGPU_PARAM_SUPPORTED_CAPSET_IDs:
        for (unsigned i = 0; i < info.capsets; ++i)
        {
            unsigned id, revision;
            size_t bytes;
            if (lume_render_capability_info(fd, i, &id, &revision, &bytes))
                return -1;
            if ((id == 1 || (id == 2 && (info.transport_features & (UINT64_C(1) << 4)))) && id < 64)
                value |= UINT64_C(1) << id;
        }
        break;
    case VIRTGPU_PARAM_RESOURCE_BLOB:
    case VIRTGPU_PARAM_HOST_VISIBLE:
    case VIRTGPU_PARAM_CROSS_DEVICE:
    case VIRTGPU_PARAM_EXPLICIT_DEBUG_NAME:
        break;
    default:
        return lume_drm_error(EINVAL);
    }
    memcpy((void *)(uintptr_t)parameter->value, &value, sizeof(value));
    return 0;
}

static int lume_drm_ioctl(int fd, unsigned long operation, void *data)
{
    if (!data)
        return lume_drm_error(EINVAL);
    switch (operation)
    {
    case DRM_IOCTL_VERSION:
    {
        struct lume_render_info info;
        if (lume_render_query(fd, &info))
            return -1;
        if (info.backend != LUME_RENDER_BACKEND_VIRGL)
            return lume_drm_error(EOPNOTSUPP);
        drm_version_t *version = data;
        version->version_major = 0;
        version->version_minor = 0;
        version->version_patchlevel = 0;
        version->name_len = lume_drm_string(version->name, version->name_len, "virtio_gpu");
        version->date_len = lume_drm_string(version->date, version->date_len, "");
        version->desc_len =
            lume_drm_string(version->desc, version->desc_len, "Lume VirGL render interface");
        return 0;
    }
    case DRM_IOCTL_GET_CAP:
    {
        struct drm_get_cap *capability = data;
        struct lume_render_info info;
        if (lume_render_query(fd, &info))
            return -1;
        capability->value = 0;
        switch (capability->capability)
        {
        case DRM_CAP_TIMESTAMP_MONOTONIC:
            capability->value = 1;
            return 0;
        case DRM_CAP_PRIME:
            if ((info.operations & (LUME_RENDER_OPERATION(LUME_RENDER_PRIME_EXPORT) |
                                    LUME_RENDER_OPERATION(LUME_RENDER_PRIME_IMPORT))) ==
                (LUME_RENDER_OPERATION(LUME_RENDER_PRIME_EXPORT) |
                 LUME_RENDER_OPERATION(LUME_RENDER_PRIME_IMPORT)))
                capability->value = DRM_PRIME_CAP_IMPORT | DRM_PRIME_CAP_EXPORT;
            return 0;
        case DRM_CAP_DUMB_BUFFER:
        case DRM_CAP_ADDFB2_MODIFIERS:
        case DRM_CAP_SYNCOBJ:
        case DRM_CAP_SYNCOBJ_TIMELINE:
            return 0;
        default:
            return lume_drm_error(EINVAL);
        }
    }
    case DRM_IOCTL_VIRTGPU_GETPARAM:
        return lume_drm_parameter(fd, data);
    case DRM_IOCTL_VIRTGPU_CONTEXT_INIT:
    {
        const struct drm_virtgpu_context_init *init = data;
        if (init->pad || init->num_params != 1 || !init->ctx_set_params ||
            init->ctx_set_params > UINTPTR_MAX)
            return lume_drm_error(EINVAL);
        struct drm_virtgpu_context_set_param parameter;
        memcpy(&parameter, (const void *)(uintptr_t)init->ctx_set_params, sizeof(parameter));
        if (parameter.param != VIRTGPU_CONTEXT_PARAM_CAPSET_ID || parameter.value < 1 ||
            parameter.value > 2)
            return lume_drm_error(EOPNOTSUPP);
        unsigned index, revision;
        size_t bytes;
        if (lume_drm_capset(fd, (unsigned)parameter.value, &index, &revision, &bytes))
            return -1;
        return lume_render_context(fd, index, (unsigned)parameter.value);
    }
    case DRM_IOCTL_VIRTGPU_GET_CAPS:
    {
        struct drm_virtgpu_get_caps *caps = data;
        if (caps->pad || !caps->addr || caps->addr > UINTPTR_MAX || !caps->size)
            return lume_drm_error(EINVAL);
        unsigned index, revision;
        size_t bytes;
        if (lume_drm_capset(fd, caps->cap_set_id, &index, &revision, &bytes))
            return -1;
        if (caps->cap_set_ver > revision || !bytes || bytes > 65512)
            return lume_drm_error(EINVAL);
        void *payload = malloc(bytes);
        if (!payload)
            return lume_drm_error(ENOMEM);
        int result =
            lume_render_capability(fd, index, caps->cap_set_id, caps->cap_set_ver, payload, bytes);
        if (!result)
            memcpy((void *)(uintptr_t)caps->addr, payload, caps->size < bytes ? caps->size : bytes);
        int error = errno;
        free(payload);
        errno = error;
        return result;
    }
    case DRM_IOCTL_VIRTGPU_RESOURCE_CREATE:
    {
        struct drm_virtgpu_resource_create *create = data;
        if (create->bo_handle || create->res_handle)
            return lume_drm_error(EINVAL);
        struct lume_render_resource resource = {
            .bytes = ((uint64_t)(create->size ? create->size : 1) + 4095) & ~UINT64_C(4095),
            .target = create->target,
            .format = create->format,
            .bind = create->bind,
            .width = create->width,
            .height = create->height,
            .depth = create->depth,
            .array_size = create->array_size,
            .last_level = create->last_level,
            .samples = create->nr_samples,
            .flags = create->flags};
        if (lume_render_resource_create(fd, &resource))
            return -1;
        create->bo_handle = create->res_handle = resource.handle;
        return 0;
    }
    case DRM_IOCTL_VIRTGPU_RESOURCE_INFO:
    {
        struct drm_virtgpu_resource_info *info = data;
        struct lume_render_resource resource;
        if (lume_render_resource_info(fd, info->bo_handle, &resource))
            return -1;
        info->res_handle = resource.handle;
        info->size = (uint32_t)resource.bytes;
        info->blob_mem = 0;
        return 0;
    }
    case DRM_IOCTL_GEM_CLOSE:
    {
        const struct drm_gem_close *close = data;
        if (close->pad)
            return lume_drm_error(EINVAL);
        return lume_render_resource_close(fd, close->handle);
    }
    case DRM_IOCTL_PRIME_HANDLE_TO_FD:
    {
        struct drm_prime_handle *prime = data;
        if (prime->flags & ~(DRM_CLOEXEC | DRM_RDWR))
            return lume_drm_error(EINVAL);
        unsigned flags = (prime->flags & DRM_CLOEXEC ? LUME_RENDER_CLOEXEC : 0) |
                         (prime->flags & DRM_RDWR ? LUME_RENDER_RDWR : 0);
        int descriptor = lume_render_prime_export(fd, prime->handle, flags);
        if (descriptor < 0)
            return -1;
        prime->fd = descriptor;
        return 0;
    }
    case DRM_IOCTL_PRIME_FD_TO_HANDLE:
    {
        struct drm_prime_handle *prime = data;
        if (prime->flags)
            return lume_drm_error(EINVAL);
        return lume_render_prime_import(fd, prime->fd, &prime->handle);
    }
    case DRM_IOCTL_VIRTGPU_TRANSFER_TO_HOST:
    case DRM_IOCTL_VIRTGPU_TRANSFER_FROM_HOST:
    {
        struct drm_virtgpu_3d_transfer_to_host transfer;
        _Static_assert(sizeof(transfer) == sizeof(struct drm_virtgpu_3d_transfer_from_host),
                       "VirtIO transfer ABI");
        memcpy(&transfer, data, sizeof(transfer));
        struct lume_render_transfer native = {.handle = transfer.bo_handle,
                                              .offset = transfer.offset,
                                              .level = transfer.level,
                                              .stride = transfer.stride,
                                              .layer_stride = transfer.layer_stride,
                                              .x = transfer.box.x,
                                              .y = transfer.box.y,
                                              .z = transfer.box.z,
                                              .width = transfer.box.w,
                                              .height = transfer.box.h,
                                              .depth = transfer.box.d};
        return lume_render_transfer(
            fd,
            operation == DRM_IOCTL_VIRTGPU_TRANSFER_FROM_HOST ? LUME_RENDER_TRANSFER_FROM_HOST : 0,
            &native);
    }
    case DRM_IOCTL_VIRTGPU_WAIT:
    {
        struct drm_virtgpu_3d_wait *wait = data;
        if (wait->flags & ~VIRTGPU_WAIT_NOWAIT)
            return lume_drm_error(EINVAL);
        int result = lume_render_resource_wait(fd, wait->handle,
                                               wait->flags & VIRTGPU_WAIT_NOWAIT ? 0 : 60000);
        if (result && errno == EAGAIN)
            errno = EBUSY;
        return result;
    }
    case DRM_IOCTL_VIRTGPU_EXECBUFFER:
    {
        const struct drm_virtgpu_execbuffer *submit = data;
        if (submit->flags || submit->ring_idx || submit->num_in_syncobjs ||
            submit->num_out_syncobjs)
            return lume_drm_error(EOPNOTSUPP);
        if (!submit->command || submit->command > UINTPTR_MAX || submit->bo_handles > UINTPTR_MAX ||
            submit->num_bo_handles > 1024 || (submit->num_bo_handles && !submit->bo_handles))
            return lume_drm_error(EINVAL);
        uint64_t fence;
        return lume_render_submit_resources(
            fd, (const void *)(uintptr_t)submit->command, submit->size,
            (const uint32_t *)(uintptr_t)submit->bo_handles, submit->num_bo_handles, &fence);
    }
    default:
        return lume_drm_error(EOPNOTSUPP);
    }
}
#endif
