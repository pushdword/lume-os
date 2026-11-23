/* SPDX-License-Identifier: BSD-3-Clause */
#include <dlfcn.h>
#include <stddef.h>

static _Thread_local const char *dynamic_error;

static void unsupported_dynamic_object(void)
{
    dynamic_error = "Dynamic shared objects are not supported";
}

void *dlopen(const char *name, int flags)
{
    (void)name;
    (void)flags;
    unsupported_dynamic_object();
    return NULL;
}

void *dlsym(void *handle, const char *name)
{
    (void)handle;
    (void)name;
    unsupported_dynamic_object();
    return NULL;
}

int dlclose(void *handle)
{
    (void)handle;
    unsupported_dynamic_object();
    return -1;
}

char *dlerror(void)
{
    const char *message = dynamic_error;
    dynamic_error = NULL;
    return (char *)message;
}
