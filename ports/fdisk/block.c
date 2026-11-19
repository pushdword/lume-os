/* SPDX-License-Identifier: BSD-3-Clause */
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include "blkdev.h"

int blkdev_get_sectors(int fd, unsigned long long *sectors)
{
    struct stat info;
    if (fstat(fd, &info))
    {
        return -1;
    }
    *sectors = (unsigned long long)info.st_size / 512;
    return 0;
}

int blkdev_get_sector_size(int fd, int *sector_size)
{
    return ioctl(fd, BLKSSZGET, sector_size);
}

int blkdev_get_geometry(int fd, unsigned int *h, unsigned int *s)
{
    (void)fd;
    (void)h;
    (void)s;
    errno = ENOTTY;
    return -1;
}
