/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_POSIX_DLFCN_H
#define LUME_POSIX_DLFCN_H

#define RTLD_LAZY 1
#define RTLD_NOW 2
#define RTLD_NOLOAD 4
#define RTLD_LOCAL 0
#define RTLD_GLOBAL 0x100
#define RTLD_NODELETE 0x1000
#define RTLD_DEFAULT ((void *)0)
#define RTLD_NEXT ((void *)-1)

#ifdef __cplusplus
extern "C"
{
#endif
    void *dlopen(const char *, int);
    void *dlsym(void *, const char *);
    int dlclose(void *);
    char *dlerror(void);
#ifdef __cplusplus
}
#endif

#endif
