#define LUME_USER_HOST_TEST 1
#define LUME_ARCH_AMD64 1
#define main installer_main
#define puts user_puts
#include "../../apps/installer/main.c"
#undef puts
#undef main
#include "sha256.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned char payload[8192], media_file[512], disk_boot[8192], disk_data[2097152];
static char output[32768];
static const char *input;
static size_t input_offset, output_bytes;
static uint64_t offsets[8];
static unsigned writes, writable_opens, directory_index;
static bool busy, replaced;

static void test_put(unsigned char *p, uint64_t value, unsigned bytes)
{
    for (unsigned i = 0; i < bytes; ++i)
        p[i] = (unsigned char)(value >> (i * 8));
}

static unsigned char *disk_region(uint64_t offset, size_t bytes)
{
    if (offset < sizeof(disk_boot) && bytes <= sizeof(disk_boot) - offset)
        return disk_boot + (size_t)offset;
    uint64_t start = LUME_INSTALL_DATA_FIRST * 512;
    assert(offset >= start && offset - start < sizeof(disk_data));
    assert(bytes <= sizeof(disk_data) - (offset - start));
    return disk_data + (size_t)(offset - start);
}

int64_t call(uint64_t operation, uint64_t a, uint64_t b, uint64_t c)
{
    if (operation == LUME_SYS_HARDWARE)
        return -LUME_ENOENT;
    if (operation == LUME_SYS_SLEEP)
        return 0;
    if (operation == LUME_SYS_OPEN)
    {
        const char *path = (const void *)(uintptr_t)a;
        unsigned fd;
        if (!strcmp(path, "/dev"))
        {
            fd = 3;
            directory_index = 0;
        }
        else if (!strcmp(path, "/dev/vda"))
        {
            fd = 4;
            if (b == LUME_O_RDWR)
            {
                ++writable_opens;
                if (busy)
                    return -LUME_EBUSY;
            }
        }
        else if (!strcmp(path, "/usr/share/install/media.bin"))
            fd = 5;
        else if (!strcmp(path, "/usr/share/install/boot.bin"))
            fd = 6;
        else
            return -LUME_ENOENT;
        offsets[fd] = 0;
        return fd;
    }
    if (operation == LUME_SYS_CLOSE)
        return 0;
    if (operation == LUME_SYS_STAT)
    {
        struct lume_stat *info = (void *)(uintptr_t)b;
        *info = (struct lume_stat){.inode = a,
                                   .size = a == 4   ? LUME_INSTALL_MIN_SECTORS * 512
                                           : a == 5 ? sizeof(media_file)
                                                    : sizeof(payload),
                                   .kind = a == 4 ? LUME_NODE_BLOCK : LUME_NODE_FILE};
        if (a == 4 && replaced && writable_opens)
            ++info->inode;
        return 0;
    }
    if (operation == LUME_SYS_GETDENTS)
    {
        assert(a == 3 && c == sizeof(struct lume_dirent));
        if (directory_index++)
            return 0;
        struct lume_dirent *entry = (void *)(uintptr_t)b;
        *entry = (struct lume_dirent){.inode = 4, .kind = LUME_NODE_BLOCK};
        memcpy(entry->name, "vda", 4);
        return sizeof(*entry);
    }
    if (operation == LUME_SYS_SEEK)
    {
        assert(a >= 4 && a <= 6 && c == LUME_SEEK_SET);
        offsets[a] = b;
        return (int64_t)b;
    }
    if (operation == LUME_SYS_READ)
    {
        void *out = (void *)(uintptr_t)b;
        if (!a)
        {
            assert(c == 1);
            if (!input[input_offset])
                return 0;
            *(unsigned char *)out = (unsigned char)input[input_offset++];
            return 1;
        }
        const unsigned char *data;
        if (a == 4)
            data = disk_region(offsets[a], (size_t)c);
        else
        {
            size_t limit = a == 5 ? sizeof(media_file) : sizeof(payload);
            assert(offsets[a] <= limit && c <= limit - offsets[a]);
            data = (a == 5 ? media_file : payload) + (size_t)offsets[a];
        }
        memcpy(out, data, (size_t)c);
        offsets[a] += c;
        return (int64_t)c;
    }
    if (operation == LUME_SYS_WRITE)
    {
        const void *data = (const void *)(uintptr_t)b;
        if (a == 1)
        {
            assert(c < sizeof(output) - output_bytes);
            memcpy(output + output_bytes, data, (size_t)c);
            output_bytes += (size_t)c;
            output[output_bytes] = 0;
            return (int64_t)c;
        }
        assert(a == 4 && writable_opens && !input[input_offset]);
        ++writes;
        memcpy(disk_region(offsets[a], (size_t)c), data, (size_t)c);
        offsets[a] += c;
        return (int64_t)c;
    }
    assert(operation == LUME_SYS_FS && a == LUME_FS_SYNC && b == 4);
    return 0;
}

static void reset_case(const char *answer)
{
    memset(payload, 0, sizeof(payload));
    memset(media_file, 0, sizeof(media_file));
    memset(disk_boot, 0, sizeof(disk_boot));
    memset(disk_data, 0, sizeof(disk_data));
    memset(offsets, 0, sizeof(offsets));
    writes = writable_opens = directory_index = 0;
    busy = replaced = false;
    output_bytes = input_offset = 0;
    output[0] = 0;
    input = answer;
    test_put(payload + 440, LUME_INSTALL_DISK_ID, 4);
    payload[510] = 0x55;
    payload[511] = 0xaa;
    memcpy(media_file, "LUMEINS1", 8);
    test_put(media_file + 8, LUME_INSTALL_VERSION, 4);
    test_put(media_file + 12, LUME_INSTALL_AMD64, 4);
    test_put(media_file + 16, sizeof(payload), 8);
    test_put(media_file + 24, LUME_INSTALL_DATA_FIRST, 8);
    struct lume_sha256 hash;
    lume_sha256_init(&hash);
    lume_sha256_update(&hash, payload, sizeof(payload));
    lume_sha256_final(&hash, media_file + 32);
}

int main(void)
{
    char *interactive[] = {"install", NULL};
    char *list[] = {"install", "--list", NULL};
    char *target[] = {"install", "/dev/vda", NULL};
    reset_case("");
    assert(installer_main(2, list) == 0 && writes == 0 && writable_opens == 0);
    reset_case("\n");
    assert(installer_main(2, target) == 0 && writes == 0 && writable_opens == 0);
    assert(strstr(output, "Installation cancelled."));
    reset_case("/dev/vda\nERASE /dev/wrong\n");
    assert(installer_main(1, interactive) == 0 && writes == 0 && writable_opens == 0);
    reset_case("/dev/vda\r\nERASE /dev/vda\r");
    assert(installer_main(1, interactive) == 0 && writes > 0 && writable_opens == 1);
    assert(strstr(output, "LumeOS installation complete."));
    assert(disk_boot[510] == 0x55 && disk_boot[511] == 0xaa);
    reset_case("ERASE /dev/vda\n");
    busy = true;
    assert(installer_main(2, target) == 1 && writes == 0);
    reset_case("ERASE /dev/vda\n");
    replaced = true;
    assert(installer_main(2, target) == 1 && writes == 0);
    reset_case("ERASE /dev/vda\n");
    payload[1024] ^= 1;
    assert(installer_main(2, target) == 1 && writes == 0 && writable_opens == 0);
    reset_case("\003");
    assert(installer_main(2, target) == 0 && writes == 0 && writable_opens == 0);
    puts("Installer disk selection, confirmation, cancellation and replacement checks passed");
    return 0;
}
