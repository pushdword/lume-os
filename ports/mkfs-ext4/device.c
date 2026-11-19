/* SPDX-License-Identifier: BSD-3-Clause */
#include "device.h"

#include <errno.h>
#include <unistd.h>

struct mkfs_device mkfs_device;

static errcode_t open_channel(const char *name, int flags, io_channel *handle)
{
    if (!(flags & IO_FLAG_RW))
        return EROFS;
    io_channel channel = calloc(1, sizeof(*channel));
    if (!channel)
        return ENOMEM;
    channel->name = strdup(name);
    if (!channel->name)
    {
        free(channel);
        return ENOMEM;
    }
    channel->magic = EXT2_ET_MAGIC_IO_CHANNEL;
    channel->manager = lume_ext4_io;
    channel->refcount = 1;
    channel->block_size = 1024;
    *handle = channel;
    return 0;
}

static errcode_t close_channel(io_channel channel)
{
    if (!--channel->refcount)
    {
        free(channel->name);
        free(channel);
    }
    return 0;
}

static errcode_t set_block_size(io_channel channel, int size)
{
    if (size != 1024 && size != 4096)
        return EINVAL;
    channel->block_size = size;
    return 0;
}

static errcode_t transfer(uint64_t offset, size_t size, void *data, int writing)
{
    if (offset > mkfs_device.byte_count || size > mkfs_device.byte_count - offset)
        return EINVAL;
    off_t position = (off_t)(mkfs_device.first_byte + offset);
    if (lseek(mkfs_device.fd, position, SEEK_SET) != position)
        return errno;
    unsigned char *bytes = data;
    while (size)
    {
        ssize_t moved =
            writing ? write(mkfs_device.fd, bytes, size) : read(mkfs_device.fd, bytes, size);
        if (moved < 0 && errno == EINTR)
            continue;
        if (moved <= 0)
            return moved ? errno : EIO;
        bytes += moved;
        size -= (size_t)moved;
    }
    return 0;
}

static errcode_t block_transfer(io_channel channel, unsigned long long block, int count, void *data,
                                int writing)
{
    unsigned size = (unsigned)channel->block_size;
    if (block > UINT64_MAX / size)
        return EOVERFLOW;
    size_t length = count < 0 ? (size_t)(-(int64_t)count) : (size_t)count * size;
    return transfer(block * size, length, data, writing);
}

static errcode_t read64(io_channel channel, unsigned long long block, int count, void *buffer)
{
    return block_transfer(channel, block, count, buffer, 0);
}

static errcode_t write64(io_channel channel, unsigned long long block, int count,
                         const void *buffer)
{
    return block_transfer(channel, block, count, (void *)buffer, 1);
}

static errcode_t read32(io_channel channel, unsigned long block, int count, void *buffer)
{
    return read64(channel, block, count, buffer);
}

static errcode_t write32(io_channel channel, unsigned long block, int count, const void *buffer)
{
    return write64(channel, block, count, buffer);
}

static errcode_t write_byte(io_channel channel, unsigned long offset, int count, const void *buffer)
{
    (void)channel;
    return count < 0 ? EINVAL : transfer(offset, (size_t)count, (void *)buffer, 1);
}

static errcode_t flush(io_channel channel)
{
    (void)channel;
    return fsync(mkfs_device.fd) ? errno : 0;
}

static struct struct_io_manager manager = {.magic = EXT2_ET_MAGIC_IO_MANAGER,
                                           .name = "Lume block I/O",
                                           .open = open_channel,
                                           .close = close_channel,
                                           .set_blksize = set_block_size,
                                           .read_blk = read32,
                                           .write_blk = write32,
                                           .flush = flush,
                                           .write_byte = write_byte,
                                           .read_blk64 = read64,
                                           .write_blk64 = write64};

io_manager lume_ext4_io = &manager;
