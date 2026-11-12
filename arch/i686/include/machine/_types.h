/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_I686_MACHINE_TYPES_H
#define LUME_I686_MACHINE_TYPES_H
#if __has_include(<lume_libc_config.h>)
#include <lume_libc_config.h>
#if LUME_LIBC_OFF_T_BITS == 64
#ifndef LUME_NATIVE_LFS64
#define LUME_NATIVE_LFS64 1
#endif
#elif defined(LUME_NATIVE_LFS64)
#error "Native 64-bit file offsets require a matching libc"
#endif
#elif defined(LUME_NATIVE_LFS64) && !defined(LUME_LIBC_BUILD)
#error "Native 64-bit file offsets require a matching libc"
#endif
#if defined(LUME_NATIVE_LFS64)
typedef __INT64_TYPE__ _off_t;
typedef __INT64_TYPE__ _fpos_t;
#define __machine_off_t_defined 1
#define __machine_fpos_t_defined 1
#endif
#endif
