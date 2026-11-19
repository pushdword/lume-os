#include "lume/install_format.h"
#include "sha256.h"

static unsigned char buffer[LUME_INSTALL_BUFFER_BYTES];
static unsigned char verify[LUME_INSTALL_BUFFER_BYTES];

static uint32_t word(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static uint64_t wide(const unsigned char *p)
{
    return word(p) | (uint64_t)word(p + 4) << 32;
}

static void put(unsigned char *p, uint32_t n, unsigned bytes)
{
    for (unsigned i = 0; i < bytes; ++i)
        p[i] = (unsigned char)(n >> (8 * i));
}

static void zero(void *data, size_t bytes)
{
    unsigned char *p = data;
    for (size_t i = 0; i < bytes; ++i)
        p[i] = 0;
}

static void copy(void *out, const void *data, size_t bytes)
{
    unsigned char *p = out;
    const unsigned char *q = data;
    for (size_t i = 0; i < bytes; ++i)
        p[i] = q[i];
}

static bool equal(const void *a, const void *b, size_t bytes)
{
    const unsigned char *p = a, *q = b;
    unsigned differences = 0;
    for (size_t i = 0; i < bytes; ++i)
        differences |= p[i] ^ q[i];
    return !differences;
}

bool lume_install_plan(const unsigned char media[512], unsigned architecture, uint64_t disk_bytes,
                       struct lume_install_plan *plan)
{
    if (!media || !plan ||
        (architecture != LUME_INSTALL_AMD64 && architecture != LUME_INSTALL_I686))
        return false;
    zero(plan, sizeof(*plan));
    bool uefi = architecture == LUME_INSTALL_AMD64 && equal(media, "LUMEINS2", 8) &&
                word(media + 8) == LUME_INSTALL_EFI_VERSION;
    uint64_t first = uefi ? LUME_INSTALL_EFI_DATA_FIRST : LUME_INSTALL_DATA_FIRST;
    uint64_t minimum = uefi ? LUME_INSTALL_EFI_MIN_SECTORS : LUME_INSTALL_MIN_SECTORS;
    if ((!uefi && (!equal(media, "LUMEINS1", 8) || word(media + 8) != LUME_INSTALL_VERSION)) ||
        word(media + 12) != architecture || wide(media + 24) != first ||
        disk_bytes % 512 || disk_bytes / 512 < minimum || disk_bytes / 512 > UINT32_MAX)
        return false;
    for (unsigned i = 64; i < 512; ++i)
        if (media[i])
            return false;
    uint64_t bytes = wide(media + 16);
    uint64_t boot_minimum = uefi ? (LUME_INSTALL_EFI_FIRST + 1) * 512 : 10 * 512;
    if (bytes < boot_minimum || bytes % 512 || bytes > first * 512)
        return false;
    plan->uefi = uefi;
    plan->boot_bytes = bytes;
    plan->disk_sectors = disk_bytes / 512;
    plan->data_first = first;
    plan->data_sectors = (uint32_t)(plan->disk_sectors - first);
    copy(plan->sha256, media + 32, 32);
    if (uefi)
        return true;
    plan->cluster_sectors = plan->data_sectors > UINT32_C(67108864) ? 64 : 8;
    uint32_t fat = 1;
    for (unsigned i = 0; i < 32; ++i)
    {
        uint32_t clusters = (plan->data_sectors - 32 - 2 * fat) / plan->cluster_sectors;
        uint32_t needed = (uint32_t)(((uint64_t)clusters + 2) * 4 + 511) / 512;
        if (needed <= fat)
        {
            if (clusters < 65525 || clusters > UINT32_C(0x0ffffff5))
                return false;
            plan->fat_sectors = fat;
            plan->clusters = clusters;
            return true;
        }
        fat = needed;
    }
    return false;
}

bool lume_install_source(const struct lume_install_io *io, const struct lume_install_plan *plan,
                         unsigned char mbr[512])
{
    if (!io || !io->source || !plan || !mbr || !io->source(io->context, 0, mbr, 512) ||
        word(mbr + 440) != (plan->uefi ? LUME_INSTALL_EFI_DISK_ID : LUME_INSTALL_DISK_ID) ||
        mbr[510] != 0x55 || mbr[511] != 0xaa)
        return false;
    if (plan->uefi)
        for (unsigned i = 0; i < 440; ++i)
            if (mbr[i])
                return false;
    for (unsigned i = 444; i < 510; ++i)
        if (mbr[i])
            return false;
    struct lume_sha256 hash;
    lume_sha256_init(&hash);
    for (uint64_t offset = 0; offset < plan->boot_bytes;)
    {
        size_t count = plan->boot_bytes - offset > sizeof(buffer)
                           ? sizeof(buffer)
                           : (size_t)(plan->boot_bytes - offset);
        if (!io->source(io->context, offset, buffer, count))
            return false;
        lume_sha256_update(&hash, buffer, count);
        offset += count;
    }
    unsigned char digest[32];
    lume_sha256_final(&hash, digest);
    return equal(digest, plan->sha256, sizeof(digest));
}

static bool write_verify(const struct lume_install_io *io, uint64_t offset, const void *data,
                         size_t bytes)
{
    return bytes <= sizeof(verify) && io->write(io->context, offset, data, bytes) &&
           io->read(io->context, offset, verify, bytes) && equal(data, verify, bytes);
}

static bool zero_sectors(const struct lume_install_io *io, uint64_t sector, uint64_t count)
{
    zero(buffer, sizeof(buffer));
    while (count)
    {
        unsigned batch = count > sizeof(buffer) / 512 ? sizeof(buffer) / 512 : (unsigned)count;
        if (!write_verify(io, sector * 512, buffer, batch * 512))
            return false;
        sector += batch;
        count -= batch;
    }
    return true;
}

static void directory_entry(unsigned char *entry, const char *name, uint32_t cluster)
{
    zero(entry, 32);
    copy(entry, name, 11);
    entry[11] = 0x10;
    put(entry + 20, cluster >> 16, 2);
    put(entry + 26, cluster & 65535, 2);
}

static bool format(const struct lume_install_io *io, const struct lume_install_plan *plan)
{
    uint64_t first = LUME_INSTALL_DATA_FIRST;
    uint64_t data = first + 32 + 2 * plan->fat_sectors;
    if (!zero_sectors(io, first, 32 + 2 * plan->fat_sectors + 6 * plan->cluster_sectors))
        return false;
    zero(buffer, 512);
    copy(buffer, "\xeb\x58\x90LUME    ", 11);
    put(buffer + 11, 512, 2);
    buffer[13] = (unsigned char)plan->cluster_sectors;
    put(buffer + 14, 32, 2);
    buffer[16] = 2;
    buffer[21] = 0xf8;
    put(buffer + 24, 63, 2);
    put(buffer + 26, 255, 2);
    put(buffer + 28, (uint32_t)first, 4);
    put(buffer + 32, plan->data_sectors, 4);
    put(buffer + 36, plan->fat_sectors, 4);
    put(buffer + 44, 2, 4);
    put(buffer + 48, 1, 2);
    put(buffer + 50, 6, 2);
    buffer[64] = 0x80;
    buffer[66] = 0x29;
    put(buffer + 67, UINT32_C(0x4c554d45), 4);
    copy(buffer + 71, "LUME_DATA  FAT32   ", 19);
    copy(buffer + 90, "\xfa\xf4\xeb\xfd", 4);
    buffer[510] = 0x55;
    buffer[511] = 0xaa;
    if (!write_verify(io, first * 512, buffer, 512) ||
        !write_verify(io, (first + 6) * 512, buffer, 512))
        return false;
    zero(buffer, 512);
    put(buffer, UINT32_C(0x41615252), 4);
    put(buffer + 484, UINT32_C(0x61417272), 4);
    put(buffer + 488, plan->clusters - 6, 4);
    put(buffer + 492, 8, 4);
    buffer[510] = 0x55;
    buffer[511] = 0xaa;
    if (!write_verify(io, (first + 1) * 512, buffer, 512) ||
        !write_verify(io, (first + 7) * 512, buffer, 512))
        return false;
    zero(buffer, 512);
    put(buffer, UINT32_C(0x0ffffff8), 4);
    for (unsigned i = 1; i < 8; ++i)
        put(buffer + i * 4, UINT32_C(0x0fffffff), 4);
    if (!write_verify(io, (first + 32) * 512, buffer, 512) ||
        !write_verify(io, (first + 32 + plan->fat_sectors) * 512, buffer, 512))
        return false;
    static const char names[][12] = {"BIN        ", "HOME       ", "TMP        ", "SYSTEM     ",
                                     "VAR        "};
    zero(buffer, 512);
    for (unsigned i = 0; i < 5; ++i)
        directory_entry(buffer + i * 32, names[i], i + 3);
    if (!write_verify(io, data * 512, buffer, 512))
        return false;
    for (unsigned i = 0; i < 5; ++i)
    {
        zero(buffer, 512);
        directory_entry(buffer, ".          ", i + 3);
        directory_entry(buffer + 32, "..         ", 0);
        if (!write_verify(io, (data + (i + 1) * plan->cluster_sectors) * 512, buffer, 512))
            return false;
    }
    return true;
}

static void partition(unsigned char *entry, bool active, unsigned char type, uint32_t first,
                      uint32_t sectors)
{
    entry[0] = active ? 0x80 : 0;
    entry[1] = entry[5] = 0xfe;
    entry[2] = entry[3] = entry[6] = entry[7] = 0xff;
    entry[4] = type;
    put(entry + 8, first, 4);
    put(entry + 12, sectors, 4);
}

static bool verify_ext4(const struct lume_install_io *io, uint64_t first, uint64_t length)
{
    static const unsigned char label[16] = "LUME_DATA";
    unsigned char super[512];
    return io->read(io->context, first + 1024, super, sizeof(super)) &&
           super[0x38] == 0x53 && super[0x39] == 0xef && word(super + 0x18) == 2 &&
           word(super + 4) == length / LUME_INSTALL_EXT4_BLOCK_BYTES &&
           (word(super + 0x5c) & 4) && word(super + 0xe0) == 8 &&
           equal(super + 0x78, label, sizeof(label));
}

static bool format_data(const struct lume_install_io *io, const struct lume_install_plan *plan)
{
    if (!plan->uefi)
        return format(io, plan);
    uint64_t first = plan->data_first * 512;
    uint64_t bytes = (uint64_t)plan->data_sectors * 512;
    bytes -= bytes % LUME_INSTALL_EXT4_BLOCK_BYTES;
    return zero_sectors(io, plan->disk_sectors - 33, 33) &&
           io->format_ext4(io->context, first, bytes) && verify_ext4(io, first, bytes);
}

static bool valid_plan(const struct lume_install_plan *plan)
{
    uint64_t minimum = plan->uefi ? LUME_INSTALL_EFI_MIN_SECTORS : LUME_INSTALL_MIN_SECTORS;
    uint64_t first = plan->uefi ? LUME_INSTALL_EFI_DATA_FIRST : LUME_INSTALL_DATA_FIRST;
    uint64_t boot_minimum = plan->uefi ? (LUME_INSTALL_EFI_FIRST + 1) * 512 : 10 * 512;
    if (plan->disk_sectors < minimum || plan->disk_sectors > UINT32_MAX ||
        plan->data_first != first || plan->disk_sectors - first != plan->data_sectors ||
        plan->boot_bytes < boot_minimum || plan->boot_bytes % 512 ||
        plan->boot_bytes > first * 512)
        return false;
    return plan->uefi ||
           (plan->fat_sectors && plan->fat_sectors < plan->data_sectors / 2 &&
            (plan->cluster_sectors == 8 || plan->cluster_sectors == 64) &&
            plan->clusters >= 65525 &&
            plan->clusters ==
                (plan->data_sectors - 32 - 2 * plan->fat_sectors) / plan->cluster_sectors &&
            ((uint64_t)plan->clusters + 2) * 4 <= (uint64_t)plan->fat_sectors * 512);
}

bool lume_install_apply(const struct lume_install_io *io, const struct lume_install_plan *plan,
                        const unsigned char mbr[512])
{
    if (!io || !io->source || !io->read || !io->write || !io->sync || !plan || !mbr ||
        !valid_plan(plan) || (plan->uefi && !io->format_ext4))
        return false;
    unsigned char final[512], checked[512];
    if (!lume_install_source(io, plan, checked) || !equal(mbr, checked, 512))
        return false;
    copy(final, mbr, 512);
    if (plan->uefi)
        partition(final + 446, true, 0xef, (uint32_t)LUME_INSTALL_EFI_FIRST,
                  (uint32_t)LUME_INSTALL_EFI_SECTORS);
    partition(final + (plan->uefi ? 462 : 446), false, plan->uefi ? 0x83 : 0x0c,
              (uint32_t)plan->data_first, plan->data_sectors);
    final[510] = final[511] = 0;
    if (!write_verify(io, 0, final, 512) || !io->sync(io->context))
        return false;
    struct lume_sha256 hash;
    lume_sha256_init(&hash);
    lume_sha256_update(&hash, mbr, 512);
    for (uint64_t offset = 512; offset < plan->boot_bytes;)
    {
        size_t count = plan->boot_bytes - offset > sizeof(buffer)
                           ? sizeof(buffer)
                           : (size_t)(plan->boot_bytes - offset);
        if (!io->source(io->context, offset, buffer, count) ||
            !write_verify(io, offset, buffer, count))
            return false;
        lume_sha256_update(&hash, buffer, count);
        offset += count;
        if (io->progress)
            io->progress(io->context, offset, plan->boot_bytes);
    }
    unsigned char digest[32];
    lume_sha256_final(&hash, digest);
    if (!equal(digest, plan->sha256, 32) || !format_data(io, plan) || !io->sync(io->context))
        return false;
    final[510] = 0x55;
    final[511] = 0xaa;
    if (!write_verify(io, 0, final, 512) || !io->sync(io->context))
    {
        final[510] = final[511] = 0;
        (void)io->write(io->context, 0, final, 512);
        (void)io->sync(io->context);
        return false;
    }
    return true;
}
