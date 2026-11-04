/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_RESOLVER_COMPAT_H
#define LUME_RESOLVER_COMPAT_H
#define weak_alias(original, name)                                                                 \
    extern __typeof(original) name __attribute__((weak, alias(#original)))
#endif
