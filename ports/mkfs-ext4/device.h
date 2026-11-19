/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_MKFS_EXT4_DEVICE_H
#define LUME_MKFS_EXT4_DEVICE_H

#include <stdint.h>
#include <sys/types.h>
#include <ext2fs/ext2fs.h>

extern struct mkfs_device
{
    int fd;
    uint64_t first_byte, byte_count;
} mkfs_device;

extern io_manager lume_ext4_io;
#endif
