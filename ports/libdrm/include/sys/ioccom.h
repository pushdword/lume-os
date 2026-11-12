/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_DRM_IOCCOM_H
#define LUME_DRM_IOCCOM_H
#include <stdint.h>
#define _IOC_NONE 0u
#define _IOC_WRITE 1u
#define _IOC_READ 2u
#define IOC_VOID _IOC_NONE
#define IOC_IN _IOC_WRITE
#define IOC_OUT _IOC_READ
#define IOC_INOUT (_IOC_READ | _IOC_WRITE)
#define _IOC(dir, group, nr, bytes)                                                                \
    (((uint32_t)(dir) << 30) | ((uint32_t)(bytes) << 16) | ((uint32_t)(group) << 8) |              \
     (uint32_t)(nr))
#define _IOC_NR(value) ((uint32_t)(value) & 255u)
#define _IOC_TYPE(value) (((uint32_t)(value) >> 8) & 255u)
#define _IOC_SIZE(value) (((uint32_t)(value) >> 16) & 16383u)
#define _IOC_DIR(value) ((uint32_t)(value) >> 30)
#define _IO(group, nr) _IOC(_IOC_NONE, group, nr, 0)
#define _IOR(group, nr, type) _IOC(_IOC_READ, group, nr, sizeof(type))
#define _IOW(group, nr, type) _IOC(_IOC_WRITE, group, nr, sizeof(type))
#define _IOWR(group, nr, type) _IOC(_IOC_READ | _IOC_WRITE, group, nr, sizeof(type))
#endif
