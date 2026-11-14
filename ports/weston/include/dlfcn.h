/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_WESTON_DLFCN_H
#define LUME_WESTON_DLFCN_H
#define RTLD_NOW 2
#define RTLD_NOLOAD 4
#define RTLD_LOCAL 0
#define RTLD_GLOBAL 256
void *dlopen(const char *, int);
void *dlsym(void *, const char *);
int dlclose(void *);
char *dlerror(void);
#endif
