/* SPDX-License-Identifier: BSD-3-Clause */
#include "dix-config.h"
#include "scrnintstr.h"
#include "glamor.h"
#include "glamor_context.h"
#include "dri3.h"
#include "present.h"
#include "lume_glamor.h"
#include "lume/abi.h"
#include "lume/process.h"
#include "lume/render_native.h"
#include "lume/syscall_native.h"
#include <epoxy/gl.h>
#include <epoxy/egl.h>
#include <gbm.h>
#include <drm_fourcc.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>

struct imported_pixmap
{
    struct imported_pixmap *next;
    PixmapPtr pixmap;
    struct gbm_bo *buffer;
    EGLImageKHR image;
};

static struct
{
    int fd, input;
    unsigned scanout, width, height;
    struct gbm_device *device;
    struct gbm_bo *front;
    EGLDisplay display;
    EGLContext context;
    EGLImageKHR image;
    GLuint texture;
    CloseScreenProcPtr close;
    DestroyPixmapProcPtr destroy;
    struct imported_pixmap *imports;
    RRCrtcPtr crtc;
    RROutputPtr output;
    RRModePtr mode;
} gpu = {.fd = -1, .input = -1};

static DevPrivateKeyRec import_key;

static const char *initialization_stage(const char *name)
{
    const char *debug = getenv("LUME_GRAPHICS_DEBUG");
    if (debug && *debug && strcmp(debug, "0"))
        ErrorF("GPU: initializing %s\n", name);
    return name;
}

static void make_current(struct glamor_context *context)
{
    if (!eglMakeCurrent(context->display, EGL_NO_SURFACE, EGL_NO_SURFACE, context->ctx))
        FatalError("Cannot activate the graphics context (0x%x)\n", eglGetError());
}

static void release_image(EGLImageKHR image, struct gbm_bo *buffer)
{
    if (image != EGL_NO_IMAGE_KHR)
        eglDestroyImageKHR(gpu.display, image);
    if (buffer)
        gbm_bo_destroy(buffer);
}

static void cleanup(void)
{
    if (gpu.texture)
        glDeleteTextures(1, &gpu.texture);
    release_image(gpu.image, gpu.front);
    if (gpu.display != EGL_NO_DISPLAY)
    {
        eglMakeCurrent(gpu.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (gpu.context != EGL_NO_CONTEXT)
            eglDestroyContext(gpu.display, gpu.context);
        eglTerminate(gpu.display);
    }
    if (gpu.device)
        gbm_device_destroy(gpu.device);
    if (gpu.fd >= 0)
        close(gpu.fd);
    memset(&gpu, 0, sizeof(gpu));
    gpu.fd = gpu.input = -1;
}

static Bool software_renderer(const char *name)
{
    static const char *const names[] = {
        "llvmpipe",    "softpipe", "swiftshader", "basic render", "software rasterizer",
        "gdi generic", "warp"};
    for (unsigned i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
        for (const char *p = name; *p; ++p)
            if (!strncasecmp(p, names[i], strlen(names[i])))
                return TRUE;
    return FALSE;
}

static GLuint texture_from_image(EGLImageKHR image)
{
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);
    if (glGetError() != GL_NO_ERROR)
    {
        glDeleteTextures(1, &texture);
        return 0;
    }
    return texture;
}

Bool lume_glamor_prepare(int input, unsigned *width, unsigned *height)
{
    struct lume_system_features features;
    if (gpu.fd >= 0 ||
        lume_native_call(LUME_SYS_FEATURES, (uintptr_t)&features, sizeof(features),
                         LUME_FEATURE_VERSION) < 0 ||
        !(features.features & LUME_FEATURE_SHARED_WORD_WAIT))
        return FALSE;
    gpu.fd = lume_render_open(LUME_RENDER_CLOEXEC);
    if (gpu.fd < 0)
        return FALSE;
    gpu.input = input;
    const char *stage = initialization_stage("render capabilities");
    struct lume_render_info info;
    uint64_t needed = LUME_RENDER_OPERATION(LUME_RENDER_PRIME_EXPORT) |
                      LUME_RENDER_OPERATION(LUME_RENDER_PRIME_IMPORT) |
                      LUME_RENDER_OPERATION(LUME_RENDER_DISPLAY) |
                      LUME_RENDER_OPERATION(LUME_RENDER_PRESENT);
    if (lume_render_query(gpu.fd, &info) || info.backend != LUME_RENDER_BACKEND_VIRGL ||
        (info.operations & needed) != needed)
        goto failed;
    struct lume_render_display mode = {0};
    stage = initialization_stage("display mode");
    for (gpu.scanout = 0; gpu.scanout < info.scanouts; ++gpu.scanout)
        if (!lume_render_display(gpu.fd, gpu.scanout, &mode) && mode.enabled && mode.width &&
            mode.height && mode.width <= 32767 && mode.height <= 32767)
            break;
    if (gpu.scanout == info.scanouts)
        goto failed;
    gpu.width = mode.width;
    gpu.height = mode.height;
    stage = initialization_stage("buffer manager");
    gpu.device = gbm_create_device(gpu.fd);
    if (!gpu.device)
        goto failed;
    stage = initialization_stage("EGL platform interface");
    if (!epoxy_has_egl_extension(EGL_NO_DISPLAY, "EGL_EXT_platform_base"))
    {
        const char *extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
        ErrorF("GPU: EGL client extensions: %s\n", extensions ? extensions : "unavailable");
        goto failed;
    }
    stage = initialization_stage("EGL display");
    gpu.display = eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_KHR, gpu.device, NULL);
    if (gpu.display == EGL_NO_DISPLAY)
        goto failed;
    stage = initialization_stage("EGL initialization");
    if (!eglInitialize(gpu.display, NULL, NULL))
        goto failed;
    stage = initialization_stage("EGL client API");
    if (!eglBindAPI(EGL_OPENGL_ES_API))
        goto failed;
    const EGLint attributes[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR, EGL_SURFACE_TYPE, 0,
                                 EGL_NONE};
    stage = initialization_stage("EGL configuration");
    EGLConfig config;
    EGLint count;
    if (!eglChooseConfig(gpu.display, attributes, &config, 1, &count) || count != 1)
        goto failed;
    const EGLint options[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    stage = initialization_stage("EGL context");
    gpu.context = eglCreateContext(gpu.display, config, EGL_NO_CONTEXT, options);
    if (gpu.context == EGL_NO_CONTEXT)
        goto failed;
    stage = initialization_stage("EGL context activation");
    if (!eglMakeCurrent(gpu.display, EGL_NO_SURFACE, EGL_NO_SURFACE, gpu.context))
        goto failed;
    const char *renderer = (const char *)glGetString(GL_RENDERER);
    stage = initialization_stage("image extensions");
    if (!renderer || software_renderer(renderer) ||
        !epoxy_has_egl_extension(gpu.display, "EGL_KHR_image_base") ||
        !epoxy_has_egl_extension(gpu.display, "EGL_KHR_gl_texture_2D_image") ||
        !epoxy_has_gl_extension("GL_OES_EGL_image"))
        goto failed;
    stage = initialization_stage("scanout buffer");
    gpu.front = gbm_bo_create(gpu.device, gpu.width, gpu.height, GBM_FORMAT_XRGB8888,
                              GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    if (!gpu.front)
        goto failed;
    stage = initialization_stage("scanout image");
    gpu.image =
        eglCreateImageKHR(gpu.display, EGL_NO_CONTEXT, EGL_NATIVE_PIXMAP_KHR, gpu.front, NULL);
    if (gpu.image == EGL_NO_IMAGE_KHR || !(gpu.texture = texture_from_image(gpu.image)))
        goto failed;
    *width = gpu.width;
    *height = gpu.height;
    ErrorF("GPU: %s, %ux%u, EGL desktop rendering\n", renderer, gpu.width, gpu.height);
    return TRUE;
failed:
    ErrorF("GPU: initialization failed at %s (EGL 0x%x, errno %d)\n", stage, eglGetError(), errno);
    cleanup();
    return FALSE;
}

static Bool destroy_pixmap(PixmapPtr pixmap)
{
    struct imported_pixmap *record = dixLookupPrivate(&pixmap->devPrivates, &import_key);
    if (record && pixmap->refcnt == 1)
    {
        struct imported_pixmap **link = &gpu.imports;
        while (*link && *link != record)
            link = &(*link)->next;
        if (*link)
            *link = record->next;
        dixSetPrivate(&pixmap->devPrivates, &import_key, NULL);
    }
    else
        record = NULL;
    Bool result = gpu.destroy(pixmap);
    if (record)
    {
        release_image(record->image, record->buffer);
        free(record);
    }
    return result;
}

static Bool close_screen(ScreenPtr screen)
{
    const struct lume_render_present disable = {.input_fd = gpu.input, .scanout = gpu.scanout};
    (void)lume_render_present(gpu.fd, &disable);
    while (gpu.imports)
    {
        struct imported_pixmap *record = gpu.imports;
        gpu.imports = record->next;
        dixSetPrivate(&record->pixmap->devPrivates, &import_key, NULL);
        release_image(record->image, record->buffer);
        free(record);
    }
    screen->CloseScreen = gpu.close;
    screen->DestroyPixmap = gpu.destroy;
    Bool result = screen->CloseScreen(screen);
    cleanup();
    return result;
}

void glamor_egl_screen_init(ScreenPtr screen, struct glamor_context *context)
{
    context->display = gpu.display;
    context->ctx = gpu.context;
    context->make_current = make_current;
    gpu.close = screen->CloseScreen;
    gpu.destroy = screen->DestroyPixmap;
    screen->CloseScreen = close_screen;
    screen->DestroyPixmap = destroy_pixmap;
    glamor_enable_dri3(screen);
}

Bool glamor_glx_screen_init(struct glamor_context *context)
{
    (void)context;
    return FALSE;
}

static Bool screen_size(ScreenPtr screen, CARD16 width, CARD16 height, CARD32 mm_width,
                        CARD32 mm_height)
{
    return width == gpu.width && height == gpu.height && mm_width == (CARD32)screen->mmWidth &&
           mm_height == (CARD32)screen->mmHeight;
}

static Bool set_crtc(ScreenPtr screen, RRCrtcPtr crtc, RRModePtr mode, int x, int y,
                     Rotation rotation, int count, RROutputPtr *outputs)
{
    (void)screen;
    return crtc == gpu.crtc && mode == gpu.mode && !x && !y && rotation == RR_Rotate_0 &&
           count == 1 && outputs[0] == gpu.output;
}

static Bool get_modes(ScreenPtr screen, Rotation *rotations)
{
    (void)screen;
    *rotations = RR_Rotate_0;
    return TRUE;
}

static Bool randr_init(ScreenPtr screen)
{
    if (!RRScreenInit(screen))
        return FALSE;
    rrScrPrivPtr state = rrGetScrPriv(screen);
    state->rrGetInfo = get_modes;
    state->rrScreenSetSize = screen_size;
    state->rrCrtcSet = set_crtc;
    gpu.crtc = RRCrtcCreate(screen, NULL);
    gpu.output = RROutputCreate(screen, "GPU", 3, NULL);
    char name[32];
    int length = snprintf(name, sizeof(name), "%ux%u", gpu.width, gpu.height);
    xRRModeInfo mode = {.width = gpu.width,
                        .height = gpu.height,
                        .hTotal = gpu.width,
                        .vTotal = gpu.height,
                        .nameLength = length};
    gpu.mode = RRModeGet(&mode, name);
    if (!gpu.crtc || !gpu.output || !gpu.mode)
        return FALSE;
    RRCrtcSetRotations(gpu.crtc, RR_Rotate_0);
    Bool result = RROutputSetCrtcs(gpu.output, &gpu.crtc, 1) &&
                  RROutputSetModes(gpu.output, &gpu.mode, 1, 1) &&
                  RROutputSetConnection(gpu.output, RR_Connected) &&
                  RRCrtcNotify(gpu.crtc, gpu.mode, 0, 0, RR_Rotate_0, NULL, 1, &gpu.output);
    RRModeDestroy(gpu.mode);
    return result;
}

static int open_client(ScreenPtr screen, RRProviderPtr provider, int *fd)
{
    (void)screen;
    if (provider)
        return BadMatch;
    *fd = lume_render_open(LUME_RENDER_CLOEXEC);
    return *fd < 0 ? BadAlloc : Success;
}

static PixmapPtr pixmap_from_fd(ScreenPtr screen, int fd, CARD16 width, CARD16 height,
                                CARD16 stride, CARD8 depth, CARD8 bpp)
{
    if (!width || !height || (depth != 24 && depth != 32) || bpp != 32 || stride < width * 4u)
        return NULL;
    struct gbm_import_fd_data data = {.fd = fd,
                                      .width = width,
                                      .height = height,
                                      .stride = stride,
                                      .format =
                                          depth == 24 ? GBM_FORMAT_XRGB8888 : GBM_FORMAT_ARGB8888};
    struct gbm_bo *buffer =
        gbm_bo_import(gpu.device, GBM_BO_IMPORT_FD, &data, GBM_BO_USE_RENDERING);
    if (!buffer)
        return NULL;
    struct lume_render_resource resource;
    if (lume_render_resource_info(gpu.fd, gbm_bo_get_handle(buffer).u32, &resource) ||
        resource.target != 2 || resource.width != width || resource.height != height ||
        resource.depth != 1 || resource.array_size != 1 || resource.last_level ||
        resource.samples || resource.format != (depth == 24 ? 2u : 1u))
    {
        gbm_bo_destroy(buffer);
        return NULL;
    }
    EGLImageKHR image =
        eglCreateImageKHR(gpu.display, EGL_NO_CONTEXT, EGL_NATIVE_PIXMAP_KHR, buffer, NULL);
    GLuint texture = image == EGL_NO_IMAGE_KHR ? 0 : texture_from_image(image);
    struct imported_pixmap *record = texture ? calloc(1, sizeof(*record)) : NULL;
    PixmapPtr pixmap =
        record ? screen->CreatePixmap(screen, 0, 0, depth, GLAMOR_CREATE_PIXMAP_NO_TEXTURE) : NULL;
    if (!pixmap || !screen->ModifyPixmapHeader(pixmap, width, height, depth, bpp, stride, NULL))
        goto failed;
    glamor_set_pixmap_type(pixmap, GLAMOR_TEXTURE_DRM);
    if (!glamor_set_pixmap_texture(pixmap, texture))
    {
        if (glIsTexture(texture))
            glDeleteTextures(1, &texture);
        texture = 0;
        goto failed;
    }
    *record = (struct imported_pixmap){gpu.imports, pixmap, buffer, image};
    gpu.imports = record;
    dixSetPrivate(&pixmap->devPrivates, &import_key, record);
    return pixmap;
failed:
    if (pixmap)
        screen->DestroyPixmap(pixmap);
    if (texture)
        glDeleteTextures(1, &texture);
    free(record);
    release_image(image, buffer);
    return NULL;
}

static int export_pixmap(PixmapPtr pixmap, uint32_t *stride, uint64_t *bytes)
{
    glFlush();
    struct imported_pixmap *record = dixLookupPrivate(&pixmap->devPrivates, &import_key);
    struct gbm_bo *buffer = record ? record->buffer : NULL;
    EGLImageKHR image = EGL_NO_IMAGE_KHR;
    Bool temporary = FALSE;
    if (!buffer && pixmap == pixmap->drawable.pScreen->GetScreenPixmap(pixmap->drawable.pScreen))
        buffer = gpu.front;
    if (!buffer)
    {
        GLuint texture = glamor_get_pixmap_texture(pixmap);
        if (!texture)
            return -1;
        image = eglCreateImageKHR(gpu.display, gpu.context, EGL_GL_TEXTURE_2D_KHR,
                                  (EGLClientBuffer)(uintptr_t)texture, NULL);
        if (image == EGL_NO_IMAGE_KHR)
            return -1;
        buffer = gbm_bo_import(gpu.device, GBM_BO_IMPORT_EGL_IMAGE, image, GBM_BO_USE_RENDERING);
        if (!buffer)
        {
            eglDestroyImageKHR(gpu.display, image);
            return -1;
        }
        temporary = TRUE;
    }
    *stride = gbm_bo_get_stride(buffer);
    *bytes = (uint64_t)*stride * gbm_bo_get_height(buffer);
    int fd = gbm_bo_get_fd(buffer);
    if (temporary)
        release_image(image, buffer);
    return fd;
}

int glamor_egl_fd_from_pixmap(ScreenPtr screen, PixmapPtr pixmap, CARD16 *stride, CARD32 *size)
{
    (void)screen;
    uint32_t pitch;
    uint64_t bytes;
    int fd = export_pixmap(pixmap, &pitch, &bytes);
    if (fd >= 0 && (pitch > UINT16_MAX || bytes > UINT32_MAX))
    {
        close(fd);
        return -1;
    }
    if (fd >= 0)
    {
        *stride = pitch;
        *size = bytes;
    }
    return fd;
}

int glamor_egl_fds_from_pixmap(ScreenPtr screen, PixmapPtr pixmap, int *fds, uint32_t *strides,
                               uint32_t *offsets, uint64_t *modifier)
{
    (void)screen;
    uint64_t bytes;
    fds[0] = export_pixmap(pixmap, strides, &bytes);
    offsets[0] = 0;
    *modifier = DRM_FORMAT_MOD_INVALID;
    return fds[0] >= 0;
}

int glamor_egl_fd_name_from_pixmap(ScreenPtr screen, PixmapPtr pixmap, CARD16 *stride, CARD32 *size)
{
    (void)screen;
    (void)pixmap;
    (void)stride;
    (void)size;
    return -1;
}

Bool lume_glamor_screen_init(ScreenPtr screen)
{
    if (!dixRegisterPrivateKey(&import_key, PRIVATE_PIXMAP, 0) ||
        !glamor_init(screen, GLAMOR_USE_EGL_SCREEN | GLAMOR_NO_DRI3))
        return FALSE;
    static const dri3_screen_info_rec interfaces = {.version = 0,
                                                    .open = open_client,
                                                    .pixmap_from_fd = pixmap_from_fd,
                                                    .fd_from_pixmap = glamor_fd_from_pixmap};
    return randr_init(screen) && dri3_screen_init(screen, &interfaces) &&
           present_screen_init(screen, NULL);
}

Bool lume_glamor_screen_resources(ScreenPtr screen)
{
    PixmapPtr pixmap = screen->GetScreenPixmap(screen);
    glamor_set_pixmap_type(pixmap, GLAMOR_TEXTURE_DRM);
    GLuint texture = gpu.texture;
    gpu.texture = 0;
    if (glamor_set_pixmap_texture(pixmap, texture))
    {
        glamor_clear_pixmap(pixmap);
        return TRUE;
    }
    if (glIsTexture(texture))
        glDeleteTextures(1, &texture);
    return FALSE;
}

Bool lume_glamor_present(ScreenPtr screen)
{
    glamor_block_handler(screen);
    const struct lume_render_present present = {.handle = gbm_bo_get_handle(gpu.front).u32,
                                                .scanout = gpu.scanout,
                                                .input_fd = gpu.input,
                                                .width = gpu.width,
                                                .height = gpu.height};
    return !lume_render_present(gpu.fd, &present);
}
