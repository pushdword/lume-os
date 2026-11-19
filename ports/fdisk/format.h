/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_FDISK_FORMAT_H
#define LUME_FDISK_FORMAT_H
#include <stdint.h>

int format_fat32(int fd, uint64_t first, uint64_t sectors);
#endif
