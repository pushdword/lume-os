#ifndef LUME_INSTALL_FORMAT_H
#define LUME_INSTALL_FORMAT_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define LUME_INSTALL_VERSION 1u
#define LUME_INSTALL_AMD64 1u
#define LUME_INSTALL_I686 2u
#define LUME_INSTALL_SECTOR_BYTES 512u
#define LUME_INSTALL_BUFFER_BYTES 65536u
#define LUME_INSTALL_DISK_ID UINT32_C(0x374d554c)
#define LUME_INSTALL_DATA_FIRST UINT64_C(1048576)
#define LUME_INSTALL_MIN_SECTORS UINT64_C(2097152)
#define LUME_INSTALL_EFI_VERSION 2u
#define LUME_INSTALL_EFI_DISK_ID UINT32_C(0x384d554c)
#define LUME_INSTALL_EFI_FIRST UINT64_C(2048)
#define LUME_INSTALL_EFI_SECTORS UINT64_C(2097152)
#define LUME_INSTALL_EFI_DATA_FIRST (LUME_INSTALL_EFI_FIRST + LUME_INSTALL_EFI_SECTORS)
#define LUME_INSTALL_EFI_MIN_SECTORS UINT64_C(4194304)
#define LUME_INSTALL_EXT4_BLOCK_BYTES UINT64_C(4096)

struct lume_install_plan
{
    uint64_t boot_bytes, disk_sectors, data_first;
    uint32_t data_sectors, fat_sectors, clusters;
    unsigned cluster_sectors;
    bool uefi;
    unsigned char sha256[32];
};

struct lume_install_io
{
    void *context;
    bool (*source)(void *, uint64_t, void *, size_t);
    bool (*read)(void *, uint64_t, void *, size_t);
    bool (*write)(void *, uint64_t, const void *, size_t);
    bool (*sync)(void *);
    void (*progress)(void *, uint64_t, uint64_t);
    bool (*format_ext4)(void *, uint64_t, uint64_t);
};

bool lume_install_plan(const unsigned char media[512], unsigned architecture, uint64_t disk_bytes,
                       struct lume_install_plan *);
bool lume_install_source(const struct lume_install_io *, const struct lume_install_plan *,
                         unsigned char mbr[512]);
bool lume_install_apply(const struct lume_install_io *, const struct lume_install_plan *,
                        const unsigned char mbr[512]);
#endif
