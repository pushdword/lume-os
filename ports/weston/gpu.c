/* SPDX-License-Identifier: BSD-3-Clause */
#include "config.h"
#include "gpu.h"
#include "libweston-internal.h"
#include <errno.h>
#include <stdlib.h>
#ifdef LUME_WESTON_GL
#include "renderer-gl/gl-renderer.h"
#include "linux-dmabuf.h"
#include "pixel-formats.h"
#include "shared/weston-drm-fourcc.h"
#include "lume/render_native.h"
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <gbm.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

extern struct gl_renderer_interface gl_renderer_interface;
#define OUTPUT_BUFFERS 2u

struct native_buffer
{
    struct linux_dmabuf_memory base;
    struct dmabuf_attributes attributes;
    struct gbm_bo *bo;
};

struct lume_weston_gpu
{
    struct weston_compositor *compositor;
    struct weston_output *output;
    struct gbm_device *device;
    struct weston_renderbuffer *buffers[OUTPUT_BUFFERS];
    unsigned handles[OUTPUT_BUFFERS], current, next, scanout, width, height;
    int fd, input;
    bool presenting;
    char name[160];
};

static bool software_renderer(const char *name)
{
    static const char *const names[] = {"llvmpipe", "softpipe",    "swiftshader",
                                        "software", "gdi generic", "warp"};
    if (!name)
        return true;
    for (unsigned i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
        for (const char *p = name; *p; ++p)
            if (!strncasecmp(p, names[i], strlen(names[i])))
                return true;
    return false;
}

static void buffer_destroy(struct linux_dmabuf_memory *base)
{
    struct native_buffer *buffer = (struct native_buffer *)base;
    close(buffer->attributes.fd[0]);
    gbm_bo_destroy(buffer->bo);
    free(buffer);
}

struct lume_weston_gpu *lume_weston_gpu_create(struct weston_compositor *compositor, int input,
                                               unsigned *width, unsigned *height)
{
    struct lume_weston_gpu *gpu = calloc(1, sizeof(*gpu));
    if (!gpu)
        return NULL;
    int failure = ENOTSUP;
    uint32_t capabilities = compositor->capabilities;
    const struct pixel_format_info *read_format = compositor->read_format;
    gpu->fd = lume_render_open(LUME_RENDER_CLOEXEC);
    gpu->input = input;
    gpu->compositor = compositor;
    if (gpu->fd < 0)
        goto fail;
    struct lume_render_info info;
    const uint64_t required = LUME_RENDER_OPERATION(LUME_RENDER_PRIME_EXPORT) |
                              LUME_RENDER_OPERATION(LUME_RENDER_PRIME_IMPORT) |
                              LUME_RENDER_OPERATION(LUME_RENDER_DISPLAY) |
                              LUME_RENDER_OPERATION(LUME_RENDER_PRESENT);
    if (lume_render_query(gpu->fd, &info) || info.backend != LUME_RENDER_BACKEND_VIRGL ||
        (info.operations & required) != required)
        goto fail;
    struct lume_render_display mode;
    for (gpu->scanout = 0; gpu->scanout < info.scanouts; ++gpu->scanout)
        if (!lume_render_display(gpu->fd, gpu->scanout, &mode) && mode.enabled && mode.width &&
            mode.height && mode.width <= 16384 && mode.height <= 16384)
            break;
    if (gpu->scanout == info.scanouts)
        goto fail;
    gpu->width = input < 0 ? mode.width : *width;
    gpu->height = input < 0 ? mode.height : *height;
    gpu->device = gbm_create_device(gpu->fd);
    if (!gpu->device)
        goto fail;
    const struct pixel_format_info *format = pixel_format_get_info(DRM_FORMAT_XRGB8888);
    const struct gl_renderer_display_options options = {.egl_platform = EGL_PLATFORM_GBM_KHR,
                                                        .egl_native_display = gpu->device,
                                                        .formats = &format,
                                                        .formats_count = 1};
    if (gl_renderer_interface.display_create(compositor, &options) < 0)
        goto fail;
    failure = EIO;
    compositor->renderer->gl = &gl_renderer_interface;
    const char *name = (const char *)glGetString(GL_RENDERER);
    if (software_renderer(name) || !compositor->renderer->create_renderbuffer_dmabuf)
    {
        compositor->renderer->destroy(compositor);
        compositor->renderer = NULL;
        goto fail;
    }
    snprintf(gpu->name, sizeof(gpu->name), "%s", name);
    *width = gpu->width;
    *height = gpu->height;
    return gpu;
fail:
    compositor->capabilities = capabilities;
    compositor->read_format = read_format;
    if (gpu->device)
        gbm_device_destroy(gpu->device);
    if (gpu->fd >= 0)
        close(gpu->fd);
    free(gpu);
    errno = failure;
    return NULL;
}

int lume_weston_gpu_output_create(struct lume_weston_gpu *gpu, struct weston_output *output)
{
    const struct gl_renderer_fbo_options options = {.fb_size = {gpu->width, gpu->height},
                                                    .area = {0, 0, gpu->width, gpu->height}};
    if (gl_renderer_interface.output_fbo_create(output, &options) < 0)
        return -1;
    gpu->output = output;
    for (unsigned i = 0; i < OUTPUT_BUFFERS; ++i)
    {
        struct native_buffer *buffer = calloc(1, sizeof(*buffer));
        if (!buffer)
            goto fail;
        buffer->attributes.fd[0] = -1;
        buffer->base.attributes = &buffer->attributes;
        buffer->base.destroy = buffer_destroy;
        buffer->bo = gbm_bo_create(gpu->device, gpu->width, gpu->height, GBM_FORMAT_XRGB8888,
                                   GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
        if (!buffer->bo)
        {
            free(buffer);
            goto fail;
        }
        buffer->attributes = (struct dmabuf_attributes){.width = gpu->width,
                                                        .height = gpu->height,
                                                        .format = DRM_FORMAT_XRGB8888,
                                                        .n_planes = 1,
                                                        .fd = {gbm_bo_get_fd(buffer->bo)},
                                                        .stride = {gbm_bo_get_stride(buffer->bo)},
                                                        .modifier = DRM_FORMAT_MOD_INVALID};
        if (buffer->attributes.fd[0] < 0)
        {
            buffer_destroy(&buffer->base);
            goto fail;
        }
        gpu->handles[i] = gbm_bo_get_handle(buffer->bo).u32;
        gpu->buffers[i] =
            output->compositor->renderer->create_renderbuffer_dmabuf(output, &buffer->base);
        if (!gpu->buffers[i])
        {
            buffer_destroy(&buffer->base);
            goto fail;
        }
    }
    return 0;
fail:
    lume_weston_gpu_output_destroy(gpu);
    return -1;
}

struct weston_renderbuffer *lume_weston_gpu_next(struct lume_weston_gpu *gpu)
{
    gpu->current = gpu->next;
    gpu->next = (gpu->next + 1) % OUTPUT_BUFFERS;
    return gpu->buffers[gpu->current];
}

int lume_weston_gpu_present(struct lume_weston_gpu *gpu)
{
    if (glGetError() != GL_NO_ERROR)
    {
        errno = EIO;
        return -1;
    }
    if (gpu->input < 0)
        return 0;
    const struct lume_render_present request = {.input_fd = gpu->input,
                                                .scanout = gpu->scanout,
                                                .handle = gpu->handles[gpu->current],
                                                .width = gpu->width,
                                                .height = gpu->height};
    if (lume_render_present(gpu->fd, &request))
        return -1;
    gpu->presenting = true;
    return 0;
}

void lume_weston_gpu_output_destroy(struct lume_weston_gpu *gpu)
{
    if (!gpu->output)
        return;
    if (gpu->presenting)
    {
        const struct lume_render_present request = {.input_fd = gpu->input,
                                                    .scanout = gpu->scanout};
        lume_render_present(gpu->fd, &request);
        gpu->presenting = false;
    }
    for (unsigned i = 0; i < OUTPUT_BUFFERS; ++i)
        if (gpu->buffers[i])
        {
            weston_renderbuffer_unref(gpu->buffers[i]);
            gpu->buffers[i] = NULL;
        }
    gl_renderer_interface.output_destroy(gpu->output);
    gpu->output = NULL;
}

void lume_weston_gpu_destroy(struct lume_weston_gpu *gpu)
{
    if (!gpu)
        return;
    gbm_device_destroy(gpu->device);
    close(gpu->fd);
    free(gpu);
}

const char *lume_weston_gpu_name(struct lume_weston_gpu *gpu)
{
    return gpu->name;
}
#else
struct lume_weston_gpu *lume_weston_gpu_create(struct weston_compositor *compositor, int input,
                                               unsigned *width, unsigned *height)
{
    (void)compositor;
    (void)input;
    (void)width;
    (void)height;
    errno = ENOTSUP;
    return NULL;
}

int lume_weston_gpu_output_create(struct lume_weston_gpu *gpu, struct weston_output *output)
{
    (void)gpu;
    (void)output;
    return -1;
}

struct weston_renderbuffer *lume_weston_gpu_next(struct lume_weston_gpu *gpu)
{
    (void)gpu;
    return NULL;
}

int lume_weston_gpu_present(struct lume_weston_gpu *gpu)
{
    (void)gpu;
    return -1;
}

void lume_weston_gpu_output_destroy(struct lume_weston_gpu *gpu)
{
    (void)gpu;
}

void lume_weston_gpu_destroy(struct lume_weston_gpu *gpu)
{
    (void)gpu;
}

const char *lume_weston_gpu_name(struct lume_weston_gpu *gpu)
{
    (void)gpu;
    return "unavailable";
}
#endif
