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
    if (!equal(media, "LUMEINS1", 8) || word(media + 8) != LUME_INSTALL_VERSION ||
        word(media + 12) != architecture || wide(media + 24) != LUME_INSTALL_DATA_FIRST ||
        disk_bytes % 512 || disk_bytes / 512 < LUME_INSTALL_MIN_SECTORS ||
        disk_bytes / 512 > UINT32_MAX)
        return false;
    for (unsigned i = 64; i < 512; ++i)
        if (media[i])
            return false;
    uint64_t bytes = wide(media + 16);
    if (bytes < 10 * 512 || bytes % 512 || bytes > LUME_INSTALL_DATA_FIRST * 512)
        return false;
    plan->boot_bytes = bytes;
    plan->disk_sectors = disk_bytes / 512;
    plan->data_sectors = (uint32_t)(plan->disk_sectors - LUME_INSTALL_DATA_FIRST);
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
            copy(plan->sha256, media + 32, 32);
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
        word(mbr + 440) != LUME_INSTALL_DISK_ID || mbr[444] || mbr[445] || mbr[510] != 0x55 ||
        mbr[511] != 0xaa)
        return false;
    for (unsigned i = 446; i < 510; ++i)
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

bool lume_install_apply(const struct lume_install_io *io, const struct lume_install_plan *plan,
                        const unsigned char mbr[512])
{
    if (!io || !io->source || !io->read || !io->write || !io->sync || !plan || !mbr ||
        plan->disk_sectors < LUME_INSTALL_MIN_SECTORS || plan->disk_sectors > UINT32_MAX ||
        plan->disk_sectors - LUME_INSTALL_DATA_FIRST != plan->data_sectors || !plan->fat_sectors ||
        plan->fat_sectors >= plan->data_sectors / 2 ||
        (plan->cluster_sectors != 8 && plan->cluster_sectors != 64) ||
        plan->boot_bytes < 10 * 512 || plan->boot_bytes % 512 ||
        plan->boot_bytes > LUME_INSTALL_DATA_FIRST * 512 || plan->clusters < 65525 ||
        plan->clusters !=
            (plan->data_sectors - 32 - 2 * plan->fat_sectors) / plan->cluster_sectors ||
        ((uint64_t)plan->clusters + 2) * 4 > (uint64_t)plan->fat_sectors * 512)
        return false;
    unsigned char final[512], checked[512];
    if (!lume_install_source(io, plan, checked) || !equal(mbr, checked, 512))
        return false;
    copy(final, mbr, 512);
    zero(final + 446, 64);
    final[447] = final[451] = 0xfe;
    final[448] = final[449] = final[452] = final[453] = 0xff;
    final[450] = 0x0c;
    put(final + 454, (uint32_t)LUME_INSTALL_DATA_FIRST, 4);
    put(final + 458, plan->data_sectors, 4);
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
    if (!equal(digest, plan->sha256, 32) || !format(io, plan) || !io->sync(io->context))
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
