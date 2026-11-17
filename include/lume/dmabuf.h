/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_DMABUF_H
#define LUME_DMABUF_H
#include <stdint.h>
#include <sys/ioccom.h>

struct dma_buf_sync
{
    uint64_t flags;
};

struct dma_buf_export_sync_file
{
    uint32_t flags;
    int32_t fd;
};

struct dma_buf_import_sync_file
{
    uint32_t flags;
    int32_t fd;
};

#define DMA_BUF_SYNC_READ UINT64_C(1)
#define DMA_BUF_SYNC_WRITE UINT64_C(2)
#define DMA_BUF_SYNC_RW (DMA_BUF_SYNC_READ | DMA_BUF_SYNC_WRITE)
#define DMA_BUF_SYNC_START UINT64_C(0)
#define DMA_BUF_SYNC_END UINT64_C(4)
#define DMA_BUF_BASE 'b'
#define DMA_BUF_IOCTL_SYNC _IOW(DMA_BUF_BASE, 0, struct dma_buf_sync)
#define DMA_BUF_IOCTL_EXPORT_SYNC_FILE _IOWR(DMA_BUF_BASE, 2, struct dma_buf_export_sync_file)
#define DMA_BUF_IOCTL_IMPORT_SYNC_FILE _IOW(DMA_BUF_BASE, 3, struct dma_buf_import_sync_file)
#endif
