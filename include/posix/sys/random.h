/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_SYS_RANDOM_H
#define LUME_SYS_RANDOM_H
#include <stddef.h>
#include <sys/types.h>

#define GRND_NONBLOCK 1u
#define GRND_RANDOM 2u

#ifdef __cplusplus
extern "C"
{
#endif
    ssize_t getrandom(void *, size_t, unsigned);
#ifdef __cplusplus
}
#endif
#endif
