/* SPDX-License-Identifier: BSD-3-Clause */
#include <stddef.h>
#include <sys/types.h>
#include <string.h>
#include "../../../vendor/openssh/openbsd-compat/sha1.h"
#ifndef BYTE_ORDER
#define BYTE_ORDER __BYTE_ORDER__
#endif
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN __ORDER_LITTLE_ENDIAN__
#endif
#define DEF_WEAK(name)
