/* SPDX-License-Identifier: BSD-3-Clause */
#include "format.h"

#include <errno.h>
#include <unistd.h>

#include "ff.h"
#include "diskio.h"

static struct
{
    int fd;
    uint64_t first;
    LBA_t sectors;
} volume;

DSTATUS disk_initialize(BYTE drive)
{
    return drive ? STA_NOINIT : 0;
}

DSTATUS disk_status(BYTE drive)
{
    return disk_initialize(drive);
}

static DRESULT transfer(BYTE drive, BYTE *data, LBA_t sector, UINT count, int writing)
{
    if (drive || !count || sector >= volume.sectors || count > volume.sectors - sector)
        return RES_PARERR;
    off_t offset = (off_t)((volume.first + sector) * 512);
    if (lseek(volume.fd, offset, SEEK_SET) != offset)
        return RES_ERROR;
    size_t remaining = (size_t)count * 512;
    while (remaining)
    {
        ssize_t result =
            writing ? write(volume.fd, data, remaining) : read(volume.fd, data, remaining);
        if (result < 0 && errno == EINTR)
            continue;
        if (result <= 0)
            return RES_ERROR;
        data += result;
        remaining -= (size_t)result;
    }
    return RES_OK;
}

DRESULT disk_read(BYTE drive, BYTE *data, LBA_t sector, UINT count)
{
    return transfer(drive, data, sector, count, 0);
}

DRESULT disk_write(BYTE drive, const BYTE *data, LBA_t sector, UINT count)
{
    return transfer(drive, (BYTE *)data, sector, count, 1);
}

DRESULT disk_ioctl(BYTE drive, BYTE operation, void *data)
{
    if (drive)
        return RES_PARERR;
    switch (operation)
    {
    case CTRL_SYNC:
        return fsync(volume.fd) ? RES_ERROR : RES_OK;
    case GET_SECTOR_COUNT:
        *(LBA_t *)data = volume.sectors;
        return RES_OK;
    case GET_BLOCK_SIZE:
        *(DWORD *)data = 1;
        return RES_OK;
    default:
        return RES_PARERR;
    }
}

int format_fat32(int fd, uint64_t first, uint64_t sectors)
{
    if (sectors > UINT32_MAX || first > INT64_MAX / 512 || sectors > INT64_MAX / 512 - first)
        return -EOVERFLOW;
    volume.fd = fd;
    volume.first = first;
    volume.sectors = (LBA_t)sectors;
    unsigned char scratch[4096];
    const MKFS_PARM options = {.fmt = FM_FAT32 | FM_SFD, .n_fat = 2};
    FRESULT result = f_mkfs("0:", &options, scratch, sizeof(scratch));
    return result == FR_OK ? 0 : result == FR_MKFS_ABORTED ? -EINVAL : -EIO;
}
