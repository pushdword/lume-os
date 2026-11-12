/* SPDX-License-Identifier: BSD-3-Clause */
#define _FILE_OFFSET_BITS 64
#include <lume/render_native.h>
#include <xf86drm.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <errno.h>
#include <sys/stat.h>
#include <gbm.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

static int egl_information(struct gbm_device *device)
{
    const char *stage = "client extensions";
    EGLDisplay display = EGL_NO_DISPLAY;
    const char *extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    printf("GPU_INFO_EGL_CLIENT %s\n", extensions ? extensions : "unavailable");
    if (!extensions)
        goto failed;
    const char *extension = strstr(extensions, "EGL_EXT_platform_base");
    const size_t length = sizeof("EGL_EXT_platform_base") - 1;
    if (!extension || (extension != extensions && extension[-1] != ' ') ||
        (extension[length] && extension[length] != ' '))
        goto failed;
    stage = "platform entry point";
    PFNEGLGETPLATFORMDISPLAYEXTPROC get_display =
        (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
    if (!get_display)
        goto failed;
    stage = "platform display";
    puts("GPU_INFO_EGL_DISPLAY_BEGIN");
    display = get_display(EGL_PLATFORM_GBM_KHR, device, NULL);
    if (display == EGL_NO_DISPLAY)
        goto failed;
    stage = "display initialization";
    puts("GPU_INFO_EGL_INITIALIZE_BEGIN");
    EGLint major = 0, minor = 0;
    if (!eglInitialize(display, &major, &minor))
        goto failed;
    printf("GPU_INFO_EGL_VERSION %d.%d\n", major, minor);
    const char *vendor = eglQueryString(display, EGL_VENDOR);
    printf("GPU_INFO_EGL_VENDOR %s\n", vendor ? vendor : "unavailable");
    if (!eglTerminate(display))
        return 1;
    puts("PASS_GPU_EGL_DISPLAY");
    return 0;
failed:
    printf("FAIL_GPU_EGL_DISPLAY %s error %x\n", stage, eglGetError());
    if (display != EGL_NO_DISPLAY)
        eglTerminate(display);
    return 1;
}

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc == 2 && !strcmp(argv[1], "--prepare"))
    {
        struct stat metadata;
        if (mkdir("/data/tmp", 0700) && errno != EEXIST)
            return 1;
        if (stat("/data/tmp", &metadata) || !S_ISDIR(metadata.st_mode))
            return 1;
        puts("PASS_GPU_DESKTOP_DIRECTORY");
        return 0;
    }
    int egl = argc == 2 && !strcmp(argv[1], "--egl");
    if (argc != 1 && !egl)
        return 2;
    int fd = open("/dev/dri/renderD128", O_RDWR);
    printf("GPU_INFO_OPEN %d errno %d\n", fd, errno);
    if (fd < 0)
        return 1;
    struct lume_render_info info;
    int result = lume_render_query(fd, &info);
    printf("GPU_INFO_QUERY %d errno %d size %u\n", result, errno, (unsigned)sizeof(info));
    if (result)
    {
        close(fd);
        return 1;
    }
    printf("GPU_INFO_BACKEND %u capsets %u operations %llu\n", info.backend, info.capsets,
           (unsigned long long)info.operations);
    int failed = 0;
    if (!info.scanouts || info.scanouts > 16)
        failed = 1;
    else
        for (unsigned index = 0; index < info.scanouts; ++index)
        {
            struct lume_render_display mode = {0};
            errno = 0;
            result = lume_render_display(fd, index, &mode);
            printf("GPU_INFO_DISPLAY %u result %d enabled %u size %ux%u errno %d\n", index, result,
                   mode.enabled, mode.width, mode.height, errno);
            if (result)
                failed = 1;
        }
    struct lume_render_pci pci;
    result = lume_render_pci(fd, &pci);
    if (result)
        failed = 1;
    printf("GPU_INFO_PCI %d errno %d\n", result, errno);
    drmVersionPtr version = drmGetVersion(fd);
    printf("GPU_INFO_DRM %s errno %d\n", version ? version->name : "unavailable", errno);
    if (version)
        drmFreeVersion(version);
    else
        failed = 1;
    struct stat metadata = {0};
    result = fstat(fd, &metadata);
    if (result || !S_ISCHR(metadata.st_mode))
        failed = 1;
    printf("GPU_INFO_FSTAT %d mode %o errno %d\n", result, metadata.st_mode, errno);
    setenv("LIBGL_DEBUG", "verbose", 1);
    struct gbm_device *device = gbm_create_device(fd);
    printf("GPU_INFO_GBM %s errno %d\n",
           device ? gbm_device_get_backend_name(device) : "unavailable", errno);
    if (device)
    {
        if (egl && egl_information(device))
            failed = 1;
        gbm_device_destroy(device);
    }
    else
        failed = 1;
    close(fd);
    if (failed)
    {
        puts("FAIL_GPU_NATIVE_INFO");
        return 1;
    }
    puts("PASS_GPU_NATIVE_INFO");
    return 0;
}
