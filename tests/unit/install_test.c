#include "lume/install_format.h"
#include "sha256.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define PAYLOAD_BYTES 8192u
#define DATA_BYTES (2u * 1024u * 1024u)

static struct fixture
{
    unsigned char payload[PAYLOAD_BYTES], boot[PAYLOAD_BYTES], data[DATA_BYTES];
    unsigned writes, syncs, fail_write, fail_sync;
    bool corrupt_read, fail_source, mutate_source;
    uint64_t last_offset;
} state;

static void put(unsigned char *p, uint64_t value, unsigned bytes)
{
    for (unsigned i = 0; i < bytes; ++i)
        p[i] = (unsigned char)(value >> (i * 8));
}

static uint32_t word(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static unsigned char *region(uint64_t offset, size_t bytes)
{
    if (offset < sizeof(state.boot) && bytes <= sizeof(state.boot) - offset)
        return state.boot + (size_t)offset;
    uint64_t start = LUME_INSTALL_DATA_FIRST * 512;
    if (offset >= start && offset - start < sizeof(state.data) &&
        bytes <= sizeof(state.data) - (offset - start))
        return state.data + (size_t)(offset - start);
    assert(!"I/O escaped the installation regions");
    return NULL;
}

static bool source(void *context, uint64_t offset, void *out, size_t bytes)
{
    assert(context == &state);
    if (state.fail_source || offset >= sizeof(state.payload) ||
        bytes > sizeof(state.payload) - offset)
        return false;
    memcpy(out, state.payload + (size_t)offset, bytes);
    if (state.mutate_source && state.writes && offset)
        ((unsigned char *)out)[0] ^= 1;
    return true;
}

static bool disk_read(void *context, uint64_t offset, void *out, size_t bytes)
{
    assert(context == &state);
    memcpy(out, region(offset, bytes), bytes);
    if (state.corrupt_read && state.writes)
        ((unsigned char *)out)[0] ^= 1;
    return true;
}

static bool disk_write(void *context, uint64_t offset, const void *data, size_t bytes)
{
    assert(context == &state);
    assert(offset % 512 == 0 && bytes % 512 == 0);
    assert(offset + bytes <= LUME_INSTALL_MIN_SECTORS * 512);
    ++state.writes;
    state.last_offset = offset;
    if (state.writes == state.fail_write)
        return false;
    memcpy(region(offset, bytes), data, bytes);
    return true;
}

static bool sync_disk(void *context)
{
    assert(context == &state);
    ++state.syncs;
    return state.syncs != state.fail_sync;
}

static const struct lume_install_io io = {&state, source, disk_read, disk_write, sync_disk, NULL};

static void fixture(unsigned char media[512])
{
    memset(&state, 0, sizeof(state));
    memset(state.boot, 0xa5, sizeof(state.boot));
    memset(state.data, 0xa5, sizeof(state.data));
    for (unsigned i = 0; i < sizeof(state.payload); ++i)
        state.payload[i] = (unsigned char)(i * 7 + 3);
    memset(state.payload + 440, 0, 70);
    put(state.payload + 440, LUME_INSTALL_DISK_ID, 4);
    state.payload[510] = 0x55;
    state.payload[511] = 0xaa;
    memset(media, 0, 512);
    memcpy(media, "LUMEINS1", 8);
    put(media + 8, LUME_INSTALL_VERSION, 4);
    put(media + 12, LUME_INSTALL_AMD64, 4);
    put(media + 16, sizeof(state.payload), 8);
    put(media + 24, LUME_INSTALL_DATA_FIRST, 8);
    struct lume_sha256 hash;
    lume_sha256_init(&hash);
    lume_sha256_update(&hash, state.payload, sizeof(state.payload));
    lume_sha256_final(&hash, media + 32);
}

static struct lume_install_plan prepare(unsigned char media[512], unsigned char mbr[512])
{
    struct lume_install_plan plan;
    fixture(media);
    assert(lume_install_plan(media, LUME_INSTALL_AMD64, LUME_INSTALL_MIN_SECTORS * 512, &plan));
    assert(lume_install_source(&io, &plan, mbr));
    assert(state.writes == 0);
    return plan;
}

static void geometry_test(void)
{
    unsigned char media[512], mbr[512];
    struct lume_install_plan plan = prepare(media, mbr);
    assert(plan.cluster_sectors == 8 && plan.clusters >= 65525);
    assert(!lume_install_plan(media, LUME_INSTALL_I686, LUME_INSTALL_MIN_SECTORS * 512, &plan));
    assert(
        !lume_install_plan(media, LUME_INSTALL_AMD64, LUME_INSTALL_MIN_SECTORS * 512 - 512, &plan));
    assert(
        !lume_install_plan(media, LUME_INSTALL_AMD64, LUME_INSTALL_MIN_SECTORS * 512 + 1, &plan));
    assert(!lume_install_plan(media, LUME_INSTALL_AMD64, (UINT64_C(1) << 32) * 512, &plan));
    assert(lume_install_plan(media, LUME_INSTALL_AMD64, (uint64_t)UINT32_MAX * 512, &plan));
    assert(plan.cluster_sectors == 64);
    media[511] = 1;
    assert(!lume_install_plan(media, LUME_INSTALL_AMD64, LUME_INSTALL_MIN_SECTORS * 512, &plan));
    media[511] = 0;
    put(media + 16, LUME_INSTALL_DATA_FIRST * 512 + 512, 8);
    assert(!lume_install_plan(media, LUME_INSTALL_AMD64, LUME_INSTALL_MIN_SECTORS * 512, &plan));
    put(media + 16, PAYLOAD_BYTES - 1, 8);
    assert(!lume_install_plan(media, LUME_INSTALL_AMD64, LUME_INSTALL_MIN_SECTORS * 512, &plan));
    assert(state.writes == 0);
}

static void source_test(void)
{
    unsigned char media[512], mbr[512];
    struct lume_install_plan plan = prepare(media, mbr);
    state.payload[1000] ^= 1;
    assert(!lume_install_source(&io, &plan, mbr));
    assert(!lume_install_apply(&io, &plan, mbr));
    assert(state.writes == 0);
    plan = prepare(media, mbr);
    state.payload[446] = 0x80;
    assert(!lume_install_source(&io, &plan, mbr));
    assert(state.writes == 0);
    plan = prepare(media, mbr);
    state.fail_source = true;
    assert(!lume_install_apply(&io, &plan, mbr));
    assert(state.writes == 0);
    plan = prepare(media, mbr);
    plan.data_sectors--;
    assert(!lume_install_apply(&io, &plan, mbr));
    assert(state.writes == 0);
}

static void installation_test(void)
{
    unsigned char media[512], mbr[512];
    struct lume_install_plan plan = prepare(media, mbr);
    assert(lume_install_apply(&io, &plan, mbr));
    assert(state.last_offset == 0 && state.syncs == 3);
    assert(state.boot[510] == 0x55 && state.boot[511] == 0xaa);
    assert(word(state.boot + 440) == LUME_INSTALL_DISK_ID);
    assert(word(state.boot + 454) == LUME_INSTALL_DATA_FIRST);
    assert(word(state.boot + 458) == plan.data_sectors);
    assert(memcmp(state.payload + 512, state.boot + 512, PAYLOAD_BYTES - 512) == 0);
    assert(word(state.data + 32) == plan.data_sectors);
    assert(word(state.data + 36) == plan.fat_sectors);
    assert(word(state.data + 67) == UINT32_C(0x4c554d45));
    assert(memcmp(state.data + 71, "LUME_DATA  ", 11) == 0);
    assert(state.data[13] == plan.cluster_sectors && state.data[510] == 0x55 &&
           state.data[511] == 0xaa);
    assert(memcmp(state.data, state.data + 6 * 512, 512) == 0);
    assert(memcmp(state.data + 512, state.data + 7 * 512, 512) == 0);
    assert(word(state.data + 512 + 488) == plan.clusters - 6);
    size_t fat = 32 * 512;
    assert(memcmp(state.data + fat, state.data + fat + plan.fat_sectors * 512,
                  plan.fat_sectors * 512) == 0);
    for (unsigned i = 1; i < 8; ++i)
        assert(word(state.data + fat + i * 4) == UINT32_C(0x0fffffff));
    size_t root = (32 + 2 * plan.fat_sectors) * 512;
    assert(memcmp(state.data + root, "BIN        ", 11) == 0);
    assert(memcmp(state.data + root + 3 * 32, "SYSTEM     ", 11) == 0);
    assert(memcmp(state.data + root + plan.cluster_sectors * 512, ".          ", 11) == 0);
    unsigned successful_writes = state.writes;
    for (unsigned failure = 1; failure <= successful_writes; ++failure)
    {
        plan = prepare(media, mbr);
        state.fail_write = failure;
        assert(!lume_install_apply(&io, &plan, mbr));
        if (failure > 1)
            assert(state.boot[510] != 0x55 || state.boot[511] != 0xaa);
    }
    for (unsigned failure = 1; failure <= 3; ++failure)
    {
        plan = prepare(media, mbr);
        state.fail_sync = failure;
        assert(!lume_install_apply(&io, &plan, mbr));
        assert(state.boot[510] != 0x55 || state.boot[511] != 0xaa);
    }
    plan = prepare(media, mbr);
    state.corrupt_read = true;
    assert(!lume_install_apply(&io, &plan, mbr));
    assert(state.boot[510] != 0x55 || state.boot[511] != 0xaa);
    plan = prepare(media, mbr);
    state.mutate_source = true;
    assert(!lume_install_apply(&io, &plan, mbr));
    assert(state.boot[510] != 0x55 || state.boot[511] != 0xaa);
}

int main(void)
{
    geometry_test();
    source_test();
    installation_test();
    puts("Installer geometry, source integrity, formatting and failure recovery passed");
    return 0;
}
