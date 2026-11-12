/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_DRM_SYSMACROS_H
#define LUME_DRM_SYSMACROS_H
#include <stdint.h>

static inline unsigned major(unsigned value)
{
    return (value >> 8) & 255u;
}

static inline unsigned minor(unsigned value)
{
    return value & 255u;
}

static inline unsigned makedev(unsigned device_major, unsigned device_minor)
{
    return ((device_major & 255u) << 8) | (device_minor & 255u);
}
#endif
