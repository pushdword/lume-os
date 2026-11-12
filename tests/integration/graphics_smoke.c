/* SPDX-License-Identifier: BSD-3-Clause */
#if defined(LUME_USE_EPOXY)
#include <epoxy/gl.h>
#include <epoxy/egl.h>
#else
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#endif
#include <gbm.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <stdint.h>
#if defined(LUME_USE_X11)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <poll.h>
#include <stdlib.h>
#endif

#if defined(LUME_USE_X11)
static unsigned image_channel(unsigned long pixel, unsigned long mask)
{
    if (!mask)
        return 256;
    while (!(mask & 1))
    {
        pixel >>= 1;
        mask >>= 1;
    }
    return (unsigned)(((uint64_t)(pixel & mask) * 255 + mask / 2) / mask);
}

static int window_pixel_ready(Display *display, Window window)
{
    for (unsigned attempt = 0; attempt < 20; ++attempt)
    {
        XImage *image = XGetImage(display, window, 32, 32, 1, 1, AllPlanes, ZPixmap);
        if (!image)
            return 0;
        unsigned long pixel = XGetPixel(image, 0, 0);
        unsigned red = image_channel(pixel, image->red_mask);
        unsigned green = image_channel(pixel, image->green_mask);
        unsigned blue = image_channel(pixel, image->blue_mask);
        XDestroyImage(image);
        if (red >= 63 && red <= 65 && green >= 127 && green <= 129 && blue >= 190 && blue <= 192)
        {
            printf("WINDOW_PIXEL %u %u %u\n", red, green, blue);
            return 1;
        }
        poll(NULL, 0, 50);
    }
    return 0;
}
#endif

static GLuint shader(GLenum type, const char *source)
{
    GLuint value = glCreateShader(type);
    glShaderSource(value, 1, &source, NULL);
    glCompileShader(value);
    GLint ready = 0;
    glGetShaderiv(value, GL_COMPILE_STATUS, &ready);
    if (!ready)
    {
        char message[1024];
        glGetShaderInfoLog(value, sizeof(message), NULL, message);
        fprintf(stderr, "Shader compilation failed: %s\n", message);
        glDeleteShader(value);
        return 0;
    }
    return value;
}

static int software_renderer(const char *renderer)
{
    static const char *const names[] = {
        "llvmpipe",    "softpipe", "swiftshader", "basic render", "software rasterizer",
        "gdi generic", "warp"};
    for (unsigned i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
        for (const char *cursor = renderer; *cursor; ++cursor)
            if (!strncasecmp(cursor, names[i], strlen(names[i])))
                return 1;
    return 0;
}

int main(int argc, char **argv)
{
    int result = 1;
    int x11 = argc == 2 && !strcmp(argv[1], "--x11");
    if (argc != 1 && !x11)
        return 2;
#if !defined(LUME_USE_X11)
    if (x11)
        return 2;
#endif
    int fd = x11 ? -1 : open("/dev/dri/renderD128", O_RDWR | O_CLOEXEC);
    if (!x11 && fd < 0)
    {
        perror("Render device");
        return 1;
    }
    struct gbm_device *device = NULL;
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLContext context = EGL_NO_CONTEXT;
    EGLSurface surface = EGL_NO_SURFACE;
#if defined(LUME_USE_X11)
    Display *native = NULL;
    Window window = 0;
    Colormap colormap = 0;
    if (x11)
    {
        if (!XInitThreads())
            goto finished;
        if (!getenv("DISPLAY"))
            setenv("DISPLAY", "unix/:0", 0);
        for (unsigned attempt = 0; attempt < 600; ++attempt)
        {
            native = XOpenDisplay(NULL);
            if (native)
                break;
            poll(NULL, 0, 100);
        }
        if (!native)
            goto finished;
        display = eglGetDisplay((EGLNativeDisplayType)native);
    }
#endif
    if (!x11)
    {
        puts("GPU render device open");
        fflush(stdout);
        device = gbm_create_device(fd);
        if (!device)
            goto finished;
        puts("GPU buffer manager ready");
        fflush(stdout);
#if defined(LUME_USE_EPOXY)
        if (!epoxy_has_egl_extension(EGL_NO_DISPLAY, "EGL_EXT_platform_base"))
            goto finished;
        display = eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_KHR, device, NULL);
#else
        display = eglGetPlatformDisplay(EGL_PLATFORM_GBM_KHR, device, NULL);
#endif
    }
    if (display == EGL_NO_DISPLAY || !eglInitialize(display, NULL, NULL) ||
        !eglBindAPI(EGL_OPENGL_ES_API))
        goto finished;
    puts("GPU EGL display ready");
    fflush(stdout);
    EGLint attributes[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR, EGL_SURFACE_TYPE,
                           x11 ? EGL_WINDOW_BIT : 0, EGL_NONE};
    EGLConfig config;
    EGLint count = 0;
    if (!eglChooseConfig(display, attributes, &config, 1, &count) || count != 1)
        goto finished;
#if defined(LUME_USE_X11)
    if (x11)
    {
        EGLint visual;
        if (!eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &visual))
            goto finished;
        XVisualInfo wanted = {.visualid = visual};
        int available;
        XVisualInfo *found = XGetVisualInfo(native, VisualIDMask, &wanted, &available);
        if (!found || !available)
        {
            if (found)
                XFree(found);
            goto finished;
        }
        colormap =
            XCreateColormap(native, RootWindow(native, found->screen), found->visual, AllocNone);
        XSetWindowAttributes settings = {.colormap = colormap};
        window = XCreateWindow(native, RootWindow(native, found->screen), 32, 32, 64, 64, 0,
                               found->depth, InputOutput, found->visual, CWColormap, &settings);
        XFree(found);
        if (!window)
            goto finished;
        XStoreName(native, window, "GPU rendering probe");
        XMapWindow(native, window);
        XSync(native, False);
        surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)window, NULL);
        if (surface == EGL_NO_SURFACE)
            goto finished;
    }
#endif
    EGLint options[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, options);
    if (context == EGL_NO_CONTEXT || !eglMakeCurrent(display, surface, surface, context))
        goto finished;
    const char *renderer = (const char *)glGetString(GL_RENDERER);
    const char *version = (const char *)glGetString(GL_VERSION);
    if (!renderer || !version || software_renderer(renderer))
    {
        fprintf(stderr, "Hardware renderer unavailable\n");
        goto finished;
    }
    printf("RENDERER %s\nVERSION %s\n", renderer, version);
    GLuint vertex =
        shader(GL_VERTEX_SHADER, "#version 300 es\n"
                                 "const vec2 p[3]=vec2[3](vec2(-1,-1),vec2(3,-1),vec2(-1,3));\n"
                                 "void main(){gl_Position=vec4(p[gl_VertexID],0,1);}\n");
    GLuint fragment =
        shader(GL_FRAGMENT_SHADER, "#version 300 es\nprecision mediump float;\nout vec4 color;\n"
                                   "void main(){color=vec4(0.25,0.5,0.75,1.0);}\n");
    if (!vertex || !fragment)
        goto finished;
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked)
        goto finished;
    GLuint texture, framebuffer;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        goto finished;
    glViewport(0, 0, 4, 4);
    glUseProgram(program);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    unsigned char pixel[4] = {0};
    glReadPixels(2, 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    if (glGetError() == GL_NO_ERROR && pixel[0] >= 63 && pixel[0] <= 65 && pixel[1] >= 127 &&
        pixel[1] <= 129 && pixel[2] >= 190 && pixel[2] <= 192 && pixel[3] == 255)
    {
        result = 0;
    }
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteTextures(1, &texture);
    if (!result && x11)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, 64, 64);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        if (glGetError() != GL_NO_ERROR || !eglSwapBuffers(display, surface))
            result = 1;
        glFinish();
        if (glGetError() != GL_NO_ERROR)
            result = 1;
#if defined(LUME_USE_X11)
        if (!result && !window_pixel_ready(native, window))
            result = 1;
#endif
    }
    glDeleteProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
finished:
    if (display != EGL_NO_DISPLAY)
    {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context != EGL_NO_CONTEXT)
            eglDestroyContext(display, context);
        if (surface != EGL_NO_SURFACE)
            eglDestroySurface(display, surface);
        eglTerminate(display);
    }
    if (device)
        gbm_device_destroy(device);
    if (fd >= 0)
        close(fd);
#if defined(LUME_USE_X11)
    if (native)
    {
        if (window)
            XDestroyWindow(native, window);
        if (colormap)
            XFreeColormap(native, colormap);
        XCloseDisplay(native);
    }
#endif
    if (result)
        fprintf(stderr, "Graphics initialization or drawing failed: EGL 0x%x\n", eglGetError());
    else
        puts(x11 ? "PASS_GPU_X11_SHADER_SWAP" : "PASS_GPU_SHADER_READBACK");
    return result;
}
