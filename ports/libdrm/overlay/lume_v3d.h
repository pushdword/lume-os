/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_LIBDRM_V3D_H
#define LUME_LIBDRM_V3D_H
#include "lume_v3d_drm.h"
#include <time.h>

static int lume_drm_v3d_wait(int fd, struct drm_syncobj_wait *wait)
{
    if (!wait->handles || wait->handles > UINTPTR_MAX || !wait->count_handles ||
        wait->count_handles > LUME_V3D_HANDLES || wait->pad ||
        (wait->flags & ~(DRM_SYNCOBJ_WAIT_FLAGS_WAIT_ALL | DRM_SYNCOBJ_WAIT_FLAGS_WAIT_FOR_SUBMIT)))
        return lume_drm_error(EINVAL);
    const uint32_t *handles = (const void *)(uintptr_t)wait->handles;
    for (;;)
    {
        unsigned ready = 0;
        for (unsigned i = 0; i < wait->count_handles; ++i)
        {
            struct lume_v3d_request native = {
                .operation = LUME_V3D_SYNC_WAIT, .handle = handles[i]};
            if (!lume_render_v3d(fd, &native))
            {
                ++ready;
                if (!(wait->flags & DRM_SYNCOBJ_WAIT_FLAGS_WAIT_ALL))
                {
                    wait->first_signaled = i;
                    return 0;
                }
            }
            else if (errno != EAGAIN)
                return -1;
        }
        if (ready == wait->count_handles)
            return 0;
        struct timespec now;
        if (clock_gettime(CLOCK_MONOTONIC, &now))
            return -1;
        int64_t nanoseconds = (int64_t)now.tv_sec * INT64_C(1000000000) + now.tv_nsec;
        if (wait->timeout_nsec <= nanoseconds)
            return lume_drm_error(ETIME);
        struct timespec delay = {.tv_nsec = 1000000};
        if (wait->timeout_nsec - nanoseconds < delay.tv_nsec)
            delay.tv_nsec = (long)(wait->timeout_nsec - nanoseconds);
        while (nanosleep(&delay, &delay) && errno == EINTR)
        {
        }
    }
}

static int lume_drm_v3d_ioctl(int fd, unsigned long operation, void *data)
{
    struct lume_v3d_request native = {0};
    switch (operation)
    {
    case DRM_IOCTL_VERSION:
    {
        drm_version_t *version = data;
        version->version_major = 1;
        version->version_minor = 0;
        version->version_patchlevel = 0;
        version->name_len = lume_drm_string(version->name, version->name_len, "v3d");
        version->date_len = lume_drm_string(version->date, version->date_len, "");
        version->desc_len = lume_drm_string(version->desc, version->desc_len,
                                             "Broadcom V3D render interface");
        return 0;
    }
    case DRM_IOCTL_GET_CAP:
    {
        struct drm_get_cap *cap = data;
        cap->value = 0;
        switch (cap->capability)
        {
        case DRM_CAP_PRIME:
            cap->value = DRM_PRIME_CAP_IMPORT | DRM_PRIME_CAP_EXPORT;
            return 0;
        case DRM_CAP_TIMESTAMP_MONOTONIC:
        case DRM_CAP_SYNCOBJ:
            cap->value = 1;
            return 0;
        case DRM_CAP_DUMB_BUFFER:
        case DRM_CAP_ADDFB2_MODIFIERS:
        case DRM_CAP_SYNCOBJ_TIMELINE:
            return 0;
        default:
            return lume_drm_error(EINVAL);
        }
    }
    case DRM_IOCTL_V3D_GET_PARAM:
    {
        struct drm_v3d_get_param *param = data;
        if (param->pad)
            return lume_drm_error(EINVAL);
        native.operation = LUME_V3D_PARAMETER;
        native.handle = param->param;
        if (lume_render_v3d(fd, &native))
            return -1;
        param->value = native.value;
        return 0;
    }
    case DRM_IOCTL_V3D_CREATE_BO:
    {
        struct drm_v3d_create_bo *bo = data;
        if (bo->flags)
            return lume_drm_error(EINVAL);
        native.operation = LUME_V3D_CREATE;
        native.size = bo->size;
        if (lume_render_v3d(fd, &native))
            return -1;
        bo->handle = native.handle;
        bo->offset = native.address;
        return 0;
    }
    case DRM_IOCTL_V3D_GET_BO_OFFSET:
    {
        struct drm_v3d_get_bo_offset *bo = data;
        native.operation = LUME_V3D_INFO;
        native.handle = bo->handle;
        if (lume_render_v3d(fd, &native))
            return -1;
        bo->offset = native.address;
        return 0;
    }
    case DRM_IOCTL_GEM_CLOSE:
    {
        const struct drm_gem_close *bo = data;
        if (bo->pad)
            return lume_drm_error(EINVAL);
        native.operation = LUME_V3D_CLOSE;
        native.handle = bo->handle;
        break;
    }
    case DRM_IOCTL_PRIME_HANDLE_TO_FD:
    {
        struct drm_prime_handle *prime = data;
        if (prime->flags & ~(DRM_CLOEXEC | DRM_RDWR))
            return lume_drm_error(EINVAL);
        native.operation = LUME_V3D_EXPORT;
        native.flags = prime->flags & DRM_RDWR ? 0 : LUME_V3D_EXPORT_READONLY;
        native.handle = prime->handle;
        if (lume_render_v3d(fd, &native))
            return -1;
        prime->fd = (int)native.value;
        if (!(prime->flags & DRM_CLOEXEC))
            (void)fcntl(prime->fd, F_SETFD, 0);
        return 0;
    }
    case DRM_IOCTL_PRIME_FD_TO_HANDLE:
    {
        struct drm_prime_handle *prime = data;
        if (prime->flags || prime->fd < 0)
            return lume_drm_error(EINVAL);
        native.operation = LUME_V3D_IMPORT;
        native.value = (unsigned)prime->fd;
        if (lume_render_v3d(fd, &native))
            return -1;
        prime->handle = native.handle;
        return 0;
    }
    case DRM_IOCTL_V3D_WAIT_BO:
    {
        const struct drm_v3d_wait_bo *wait = data;
        if (wait->pad)
            return lume_drm_error(EINVAL);
        native.operation = LUME_V3D_WAIT;
        native.handle = wait->handle;
        native.timeout = wait->timeout_ns;
        break;
    }
    case DRM_IOCTL_V3D_SUBMIT_CL:
    {
        const struct drm_v3d_submit_cl *cl = data;
        if (cl->pad || cl->perfmon_id || cl->extensions ||
            (cl->flags & ~DRM_V3D_SUBMIT_CL_FLUSH_CACHE))
            return lume_drm_error(EOPNOTSUPP);
        native.operation = LUME_V3D_SUBMIT_CL;
        native.flags = cl->flags;
        native.registers[0] = cl->bcl_start;
        native.registers[1] = cl->bcl_end;
        native.registers[2] = cl->rcl_start;
        native.registers[3] = cl->rcl_end;
        native.registers[4] = cl->qma;
        native.registers[5] = cl->qms;
        native.registers[6] = cl->qts;
        native.in_sync[0] = cl->in_sync_bcl;
        native.in_sync[1] = cl->in_sync_rcl;
        native.out_sync = cl->out_sync;
        native.count = cl->bo_handle_count;
        native.handles = cl->bo_handles;
        break;
    }
    case DRM_IOCTL_V3D_SUBMIT_TFU:
    {
        const struct drm_v3d_submit_tfu *tfu = data;
        if (tfu->flags || tfu->extensions || tfu->v71.pad)
            return lume_drm_error(EOPNOTSUPP);
        native.operation = LUME_V3D_SUBMIT_TFU;
        native.registers[0] = tfu->icfg;
        native.registers[1] = tfu->iia;
        native.registers[2] = tfu->iis;
        native.registers[3] = tfu->ica;
        native.registers[4] = tfu->iua;
        native.registers[5] = tfu->ioa;
        native.registers[6] = tfu->ios;
        memcpy(native.registers + 7, tfu->coef, sizeof(tfu->coef));
        native.registers[11] = tfu->v71.ioc;
        native.in_sync[0] = tfu->in_sync;
        native.out_sync = tfu->out_sync;
        uint32_t handles[4];
        for (unsigned i = 0; i < 4; ++i)
            if (tfu->bo_handles[i])
                handles[native.count++] = tfu->bo_handles[i];
        native.handles = (uintptr_t)handles;
        return lume_render_v3d(fd, &native);
    }
    case DRM_IOCTL_V3D_SUBMIT_CSD:
    {
        const struct drm_v3d_submit_csd *csd = data;
        if (csd->flags || csd->pad || csd->perfmon_id || csd->extensions)
            return lume_drm_error(EOPNOTSUPP);
        native.operation = LUME_V3D_SUBMIT_CSD;
        memcpy(native.registers, csd->cfg, sizeof(csd->cfg));
        native.in_sync[0] = csd->in_sync;
        native.out_sync = csd->out_sync;
        native.count = csd->bo_handle_count;
        native.handles = csd->bo_handles;
        break;
    }
    case DRM_IOCTL_SYNCOBJ_CREATE:
    {
        struct drm_syncobj_create *create = data;
        if (create->flags & ~DRM_SYNCOBJ_CREATE_SIGNALED)
            return lume_drm_error(EINVAL);
        native.operation = LUME_V3D_SYNC_CREATE;
        native.flags = create->flags;
        if (lume_render_v3d(fd, &native))
            return -1;
        create->handle = native.handle;
        return 0;
    }
    case DRM_IOCTL_SYNCOBJ_DESTROY:
    {
        const struct drm_syncobj_destroy *destroy = data;
        if (destroy->pad)
            return lume_drm_error(EINVAL);
        native.operation = LUME_V3D_SYNC_DESTROY;
        native.handle = destroy->handle;
        break;
    }
    case DRM_IOCTL_SYNCOBJ_HANDLE_TO_FD:
    case DRM_IOCTL_SYNCOBJ_FD_TO_HANDLE:
    {
        struct drm_syncobj_handle *handle = data;
        if (handle->pad || handle->point || handle->flags != 1)
            return lume_drm_error(EOPNOTSUPP);
        native.handle = handle->handle;
        if (operation == DRM_IOCTL_SYNCOBJ_HANDLE_TO_FD)
        {
            native.operation = LUME_V3D_SYNC_EXPORT;
            if (lume_render_v3d(fd, &native))
                return -1;
            handle->fd = (int)native.value;
            return 0;
        }
        if (handle->fd < 0)
            return lume_drm_error(EINVAL);
        native.operation = LUME_V3D_SYNC_IMPORT;
        native.value = (unsigned)handle->fd;
        break;
    }
    case DRM_IOCTL_SYNCOBJ_WAIT:
        return lume_drm_v3d_wait(fd, data);
    case DRM_IOCTL_SYNCOBJ_SIGNAL:
    case DRM_IOCTL_SYNCOBJ_RESET:
    {
        const struct drm_syncobj_array *array = data;
        if (!array->handles || array->handles > UINTPTR_MAX || !array->count_handles ||
            array->count_handles > LUME_V3D_HANDLES || array->pad)
            return lume_drm_error(EINVAL);
        const uint32_t *handles = (const void *)(uintptr_t)array->handles;
        for (unsigned i = 0; i < array->count_handles; ++i)
        {
            native.operation = operation == DRM_IOCTL_SYNCOBJ_SIGNAL ? LUME_V3D_SYNC_SIGNAL
                                                                      : LUME_V3D_SYNC_RESET;
            native.handle = handles[i];
            if (lume_render_v3d(fd, &native))
                return -1;
        }
        return 0;
    }
    default:
        return lume_drm_error(EOPNOTSUPP);
    }
    return lume_render_v3d(fd, &native);
}
#endif
