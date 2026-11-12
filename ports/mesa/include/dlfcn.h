/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_MESA_DLFCN_H
#define LUME_MESA_DLFCN_H
#include <stddef.h>
#define RTLD_LAZY 1
#define RTLD_NOW 2
#define RTLD_LOCAL 0
#define RTLD_GLOBAL 256
#define RTLD_DEFAULT ((void *)0)

/* Gallium and EGL are linked into the executable. External plugins cannot load. */
static inline void *dlopen(const char *name, int flags)
{
    (void)name;
    (void)flags;
    return NULL;
}

static inline void *dlsym(void *library, const char *name)
{
    (void)library;
    (void)name;
    return NULL;
}

static inline int dlclose(void *library)
{
    (void)library;
    return 0;
}

static inline char *dlerror(void)
{
    static char message[] = "External graphics plugins are unavailable";
    return message;
}
#endif
