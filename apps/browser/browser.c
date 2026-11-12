/* SPDX-License-Identifier: BSD-3-Clause */
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#if defined(__aarch64__) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
#include "lume/abi.h"
#include "lume/process.h"
#include "lume/render.h"
#include "lume/syscall_native.h"
#endif

static int owned_directory(const char *path, unsigned private)
{
    struct stat info;
    if (stat(path, &info))
        return -1;
    if (!S_ISDIR(info.st_mode) || info.st_uid != geteuid() ||
        (info.st_mode & (private ? 0077 : 0022)))
    {
        errno = EACCES;
        return -1;
    }
    return 0;
}

static int chromium_temporary_directory(const char *home)
{
    char root[PATH_MAX], cache[PATH_MAX], temporary[PATH_MAX];
    if (!realpath(home && *home ? home : "/data", root))
        return -1;
    if (strncmp(root, "/data", 5) || (root[5] && root[5] != '/') || owned_directory(root, 0))
    {
        errno = EACCES;
        return -1;
    }
    int bytes = snprintf(cache, sizeof(cache), "%s/.cache", root);
    if (bytes < 0 || (size_t)bytes >= sizeof(cache))
    {
        errno = ENAMETOOLONG;
        return -1;
    }
    if ((mkdir(cache, 0700) && errno != EEXIST) || owned_directory(cache, 0))
        return -1;
    bytes = snprintf(temporary, sizeof(temporary), "%s/chromium-tmp", cache);
    if (bytes < 0 || (size_t)bytes >= sizeof(temporary))
    {
        errno = ENAMETOOLONG;
        return -1;
    }
    if ((mkdir(temporary, 0700) && errno != EEXIST) || owned_directory(temporary, 1))
        return -1;
    return setenv("TMPDIR", temporary, 1);
}

#if defined(LUME_ARCH_I686)
static int chromium_cpu_ready(void)
{
    uint32_t a = 1, b, c = 0, d;
    __asm__ volatile("cpuid" : "+a"(a), "=b"(b), "+c"(c), "=d"(d));
    if (!(c & 1u) || (d & (UINT32_C(3) << 24)) != (UINT32_C(3) << 24))
    {
        fputs("browser: Chromium requires an SSE3 processor\n", stderr);
        return 0;
    }
    struct lume_system_features features;
    if (lume_native_call(LUME_SYS_FEATURES, (uintptr_t)&features, sizeof(features),
                         LUME_FEATURE_VERSION) ||
        !(features.features & LUME_FEATURE_SIMD_CONTEXT))
    {
        fputs("browser: Chromium requires kernel SIMD context support\n", stderr);
        return 0;
    }
    return 1;
}
#endif

static int native_graphics_ready(void)
{
#if defined(__aarch64__) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
    const char *display = getenv("DISPLAY"), *accelerated = getenv("LUME_GL_DISPLAY");
    if (!display || !*display || !accelerated || strcmp(display, accelerated))
        return 0;
    int configuration =
        open("/data/libexec/chromium/graphics.conf", O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (configuration < 0)
        return 0;
    struct stat64 metadata;
    char marker[7];
    int enabled = !fstat64(configuration, &metadata) && S_ISREG(metadata.st_mode) &&
                  metadata.st_size == 6 && read(configuration, marker, sizeof(marker)) == 6 &&
                  !memcmp(marker, "egl=1\n", 6);
    close(configuration);
    if (!enabled)
        return 0;
    struct lume_system_features features = {0};
    if (lume_native_call(LUME_SYS_FEATURES, (uintptr_t)&features, sizeof(features),
                         LUME_FEATURE_VERSION) < 0 ||
        !(features.features & LUME_FEATURE_SHARED_WORD_WAIT))
        return 0;
    int fd = open("/dev/dri/renderD128", O_RDWR | O_CLOEXEC);
    if (fd < 0)
        return 0;
    struct lume_render_info info = {0};
    struct lume_render_request query = {.version = LUME_RENDER_VERSION,
                                        .bytes = sizeof(query),
                                        .fd = fd,
                                        .data = (uintptr_t)&info,
                                        .length = sizeof(info)};
    int64_t result =
        lume_native_call(LUME_SYS_GPU, LUME_RENDER_INFO, (uintptr_t)&query, sizeof(query));
    close(fd);
    uint64_t needed =
        LUME_RENDER_OPERATION(LUME_RENDER_CONTEXT) |
        LUME_RENDER_OPERATION(LUME_RENDER_RESOURCE_CREATE) |
        LUME_RENDER_OPERATION(LUME_RENDER_TRANSFER) | LUME_RENDER_OPERATION(LUME_RENDER_SUBMIT) |
        LUME_RENDER_OPERATION(LUME_RENDER_WAIT) | LUME_RENDER_OPERATION(LUME_RENDER_PRIME_IMPORT) |
        LUME_RENDER_OPERATION(LUME_RENDER_PRIME_EXPORT) |
        LUME_RENDER_OPERATION(LUME_RENDER_DISPLAY) | LUME_RENDER_OPERATION(LUME_RENDER_PRESENT);
    return !result && info.version == LUME_RENDER_VERSION && info.bytes == sizeof(info) &&
           info.backend == LUME_RENDER_BACKEND_VIRGL && (info.operations & needed) == needed;
#else
    return 0;
#endif
}

int main(int argc, char **argv)
{
    const char *home = getenv("HOME");
    if (home && *home && mkdir(home, 0700) && errno != EEXIST)
        perror("browser: home");
    struct timespec utc;
    if (clock_gettime(CLOCK_REALTIME, &utc))
    {
        pid_t child = fork();
        if (!child)
        {
            execlp("clock", "clock", "sync", (char *)NULL);
            _exit(127);
        }
        if (child > 0)
        {
            int status;
            while (waitpid(child, &status, 0) < 0 && errno == EINTR)
            {
            }
        }
    }
    if (mkdir("/data/downloads", 0700) && errno != EEXIST)
        perror("browser: downloads");
    const char *chromium = "/data/libexec/chromium/chromium";
    if (!access(chromium, X_OK))
    {
#if defined(LUME_ARCH_I686)
        if (!chromium_cpu_ready())
            return 1;
#endif
        if (clock_gettime(CLOCK_REALTIME, &utc))
        {
            perror("browser: system time");
            return 1;
        }
        if (chromium_temporary_directory(home))
        {
            perror("browser: temporary directory");
            return 1;
        }
        if (setenv("FONTCONFIG_FILE", "/data/libexec/chromium/fonts.conf", 1) ||
            setenv("FONTCONFIG_PATH", "/data/libexec/chromium", 1))
        {
            perror("browser: font configuration");
            return 1;
        }
        int graphics = native_graphics_ready();
        const char *options[] = {"--ozone-platform=x11",
                                 graphics ? "--use-gl=egl" : "--disable-gpu", "--no-zygote",
                                 "--enable-gpu-rasterization"};
        size_t option_count = graphics ? 4 : 3;
        char **arguments = calloc((size_t)argc + option_count + 1, sizeof(*arguments));
        if (!arguments)
        {
            perror("browser: arguments");
            return 1;
        }
        arguments[0] = "chromium";
        for (size_t i = 0; i < option_count; ++i)
            arguments[i + 1] = (char *)options[i];
        for (int i = 1; i < argc; ++i)
            arguments[i + option_count] = argv[i];
        execv(chromium, arguments);
        perror("browser: chromium");
        free(arguments);
        return 1;
    }
    argv[0] = "dillo";
    (void)argc;
    execvp("dillo", argv);
    perror("browser: dillo");
    return 1;
}
