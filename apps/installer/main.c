#include "lume/app_support.h"
#include "lume/install_format.h"
#include "lume/hardware_abi.h"
#include "lume/process.h"

#if defined(LUME_ARCH_AMD64)
#define INSTALL_ARCH LUME_INSTALL_AMD64
#elif defined(LUME_ARCH_I686)
#define INSTALL_ARCH LUME_INSTALL_I686
#else
#define INSTALL_ARCH 0u
#endif

struct installation
{
    int source, target;
    unsigned progress;
};

static const char *decimal(char text[21], uint64_t value)
{
    char *end = text + 20;
    *end = 0;
    do
    {
        *--end = (char)('0' + value % 10);
        value /= 10;
    } while (value);
    return end;
}

static void number(uint64_t value)
{
    char text[21];
    puts(decimal(text, value));
}

static bool stat_fd(int fd, struct lume_stat *info)
{
    return call(LUME_SYS_STAT, (unsigned)fd, (uintptr_t)info, 0) == 0;
}

static bool transfer(int fd, uint64_t offset, void *data, size_t bytes, bool writing)
{
    if (offset > INT64_MAX ||
        call(LUME_SYS_SEEK, (unsigned)fd, offset, LUME_SEEK_SET) != (int64_t)offset)
        return false;
    unsigned char *p = data;
    while (bytes)
    {
        int64_t count = writing ? write(fd, p, bytes) : read(fd, p, bytes);
        if (count == -4)
            continue;
        if (count <= 0 || (uint64_t)count > bytes)
            return false;
        p += count;
        bytes -= (size_t)count;
    }
    return true;
}

static bool source(void *context, uint64_t offset, void *data, size_t bytes)
{
    struct installation *state = context;
    return transfer(state->source, offset, data, bytes, false);
}

static bool target_read(void *context, uint64_t offset, void *data, size_t bytes)
{
    struct installation *state = context;
    return transfer(state->target, offset, data, bytes, false);
}

static bool target_write(void *context, uint64_t offset, const void *data, size_t bytes)
{
    struct installation *state = context;
    return transfer(state->target, offset, (void *)data, bytes, true);
}

static bool target_sync(void *context)
{
    struct installation *state = context;
    return call(LUME_SYS_FS, LUME_FS_SYNC, (unsigned)state->target, 0) == 0;
}

#if defined(LUME_ARCH_AMD64)
static bool format_ext4(void *context, uint64_t first, uint64_t bytes)
{
    struct installation *state = context;
    static const char program[] = "/usr/share/install/mkfs.ext4";
    char descriptor[21], offset[21], length[21];
    const char *const args[] = {program, "--write", "-L", "LUME_DATA",
                                "--offset", decimal(offset, first), "--length",
                                decimal(length, bytes), "--fd",
                                decimal(descriptor, (unsigned)state->target), NULL};
    const struct lume_run_request request = {.path = (uintptr_t)program,
                                             .argv = (uintptr_t)args,
                                             .argc = sizeof(args) / sizeof(*args) - 1};
    puts("Creating journaled ext4 data partition...\n");
    int64_t pid = call(LUME_SYS_RUN_FDS, (uintptr_t)&request, 0, 0);
    if (pid <= 0)
    {
        puts("Could not start the ext4 formatter. Error ");
        number((uint64_t)-pid);
        puts(".\n");
        return false;
    }
    int64_t status;
    do
        status = call(LUME_SYS_WAIT, (uint64_t)pid, LUME_WAIT_BLOCK, 1);
    while (status == -LUME_EINTR);
    return status >= 0 && (uint64_t)status >> 32 == (uint64_t)pid && !(uint32_t)status;
}
#endif

static void progress(void *context, uint64_t copied, uint64_t total)
{
    struct installation *state = context;
    unsigned percent = (unsigned)(copied * 100 / total);
    if (percent / 10 > state->progress / 10)
    {
        puts("Copying and verifying system: ");
        number(percent);
        puts("%\n");
        state->progress = percent;
    }
}

static bool line(char *text, size_t capacity)
{
    static bool after_return;
    size_t used = 0;
    bool overflow = false;
    for (;;)
    {
        unsigned char c;
        int64_t count = read(0, &c, 1);
        if (count == -LUME_EAGAIN || count == -4)
        {
            (void)call(LUME_SYS_SLEEP, 10, 0, 0);
            continue;
        }
        if (count != 1)
            return false;
        if (after_return && c == '\n')
        {
            after_return = false;
            continue;
        }
        after_return = c == '\r';
        if (c == 3 || c == 4)
        {
            puts("\n");
            return false;
        }
        if (c == '\r' || c == '\n')
        {
            puts("\n");
            text[used] = 0;
            return !overflow;
        }
        if (c == 8 || c == 127)
        {
            if (used)
            {
                --used;
                puts("\b \b");
            }
        }
        else if (c >= 32 && c < 127)
        {
            if (used + 1 < capacity)
            {
                text[used++] = (char)c;
                (void)write_all(1, &c, 1);
            }
            else
                overflow = true;
        }
    }
}

static void hardware(void)
{
    puts("Hardware inventory\n");
    struct lume_hardware_request request = {.version = LUME_HARDWARE_VERSION,
                                            .operation = LUME_HARDWARE_LIST};
    for (request.index = 0; request.index < 256; ++request.index)
    {
        int64_t result = call(LUME_SYS_HARDWARE, (uintptr_t)&request, sizeof(request), 0);
        if (result)
            break;
        puts("  ");
        puts(request.info.path);
        puts("  ");
        puts(request.info.driver[0] ? request.info.driver : "no driver");
        puts("\n");
    }
}

static int disks(void)
{
    int fd = (int)open("/dev", LUME_O_RDONLY);
    if (fd < 0)
        return 0;
    puts("Disks\n");
    unsigned found = 0;
    struct lume_dirent entry;
    for (unsigned i = 0; i < 256; ++i)
    {
        int64_t count = call(LUME_SYS_GETDENTS, (unsigned)fd, (uintptr_t)&entry, sizeof(entry));
        if (count != sizeof(entry))
            break;
        if (entry.kind != LUME_NODE_BLOCK)
            continue;
        char path[LUME_PATH_MAX] = "/dev/";
        size_t name_bytes = len(entry.name);
        if (!name_bytes || name_bytes > LUME_NAME_MAX)
            continue;
        for (size_t j = 0; j <= name_bytes; ++j)
            path[j + 5] = entry.name[j];
        int disk = (int)open(path, LUME_O_RDONLY);
        struct lume_stat info;
        if (disk < 0)
            continue;
        if (stat_fd(disk, &info))
        {
            ++found;
            puts("  ");
            puts(path);
            puts("  ");
            number(info.size / (1024 * 1024));
            puts(" MiB");
            if (info.size / 512 < LUME_INSTALL_MIN_SECTORS)
                puts(" (too small)");
            puts("\n");
        }
        (void)close(disk);
    }
    (void)close(fd);
    return (int)found;
}

static bool target_path(const char *path)
{
    if (len(path) < 6 || len(path) >= LUME_PATH_MAX)
        return false;
    static const char prefix[] = "/dev/";
    for (unsigned i = 0; i < 5; ++i)
        if (path[i] != prefix[i])
            return false;
    for (const char *p = path + 5; *p; ++p)
        if (*p == '/' || *p == '.' || *p == ' ')
            return false;
    return true;
}

static bool same(const void *a, const void *b, size_t count)
{
    const unsigned char *p = a, *q = b;
    for (size_t i = 0; i < count; ++i)
        if (p[i] != q[i])
            return false;
    return true;
}

static int install(const char *path)
{
    struct installation state = {.source = -1, .target = -1};
    struct lume_install_io io = {.context = &state,
                                 .source = source,
                                 .read = target_read,
                                 .write = target_write,
                                 .sync = target_sync,
                                 .progress = progress};
#if defined(LUME_ARCH_AMD64)
    io.format_ext4 = format_ext4;
#endif
    struct lume_stat target_info, source_info, current_info;
    unsigned char media[512], mbr[512], previous[512], current[512];
    struct lume_install_plan plan;
    int result = 1;
    int manifest = (int)open("/usr/share/install/media.bin", LUME_O_RDONLY);
    if (manifest < 0)
    {
        puts("Installer media is unavailable. Boot the LumeOS installation ISO.\n");
        return 1;
    }
    bool valid = stat_fd(manifest, &source_info) && source_info.kind == LUME_NODE_FILE &&
                 source_info.size == sizeof(media) &&
                 transfer(manifest, 0, media, sizeof(media), false);
    (void)close(manifest);
    if (!valid || !target_path(path))
    {
        puts("Invalid installation media or disk path.\n");
        return 1;
    }
    state.target = (int)open(path, LUME_O_RDONLY);
    if (state.target < 0 || !stat_fd(state.target, &target_info) ||
        target_info.kind != LUME_NODE_BLOCK ||
        !lume_install_plan(media, INSTALL_ARCH, target_info.size, &plan) ||
        !target_read(&state, 0, previous, sizeof(previous)))
    {
        puts("Unsupported disk size or installation media. The minimum is 1 GiB for BIOS and 2 GiB for UEFI.\n");
        goto finish;
    }
    state.source = (int)open("/usr/share/install/boot.bin", LUME_O_RDONLY);
    if (state.source < 0 || !stat_fd(state.source, &source_info) ||
        source_info.kind != LUME_NODE_FILE || source_info.size != plan.boot_bytes)
    {
        puts("The installation payload is missing or has an invalid size.\n");
        goto finish;
    }
    puts("Checking installation payload...\n");
    if (!lume_install_source(&io, &plan, mbr))
    {
        puts("Installation payload verification failed.\n");
        goto finish;
    }
    (void)close(state.target);
    state.target = -1;
    puts("Target: ");
    puts(path);
    puts(" (");
    number(target_info.size / (1024 * 1024));
    puts(" MiB)\nThe existing disk layout and filesystems will be replaced.\n");
    puts(plan.uefi ? "Layout: UEFI system partition 1024 MiB, journaled ext4 data partition "
                   : "Layout: BIOS boot area 512 MiB, FAT32 data partition ");
    number((uint64_t)plan.data_sectors * 512 / (1024 * 1024));
    puts(" MiB.\nType ERASE ");
    puts(path);
    puts(" to install, or press Enter to cancel:\n> ");
    char answer[192], expected[192] = "ERASE ";
    size_t bytes = len(path);
    for (size_t i = 0; i <= bytes; ++i)
        expected[i + 6] = path[i];
    if (!line(answer, sizeof(answer)) || !eq(answer, expected))
    {
        puts("Installation cancelled.\n");
        result = 0;
        goto finish;
    }
    state.target = (int)open(path, LUME_O_RDWR);
    if (state.target < 0 || !stat_fd(state.target, &current_info) ||
        !same(&target_info, &current_info, sizeof(target_info)) ||
        !target_read(&state, 0, current, sizeof(current)) ||
        !same(previous, current, sizeof(previous)))
    {
        puts("The disk changed or does not permit raw writes. Installation stopped.\n");
        goto finish;
    }
    puts("Installing LumeOS...\n");
    if (!lume_install_apply(&io, &plan, mbr))
    {
        puts("Installation failed. The disk may contain an incomplete installation.\n");
        goto finish;
    }
    puts("LumeOS installation complete.\nRemove the installation media and restart the machine.\n");
    result = 0;
finish:
    if (state.source >= 0 && close(state.source) < 0)
        result = 1;
    if (state.target >= 0 && close(state.target) < 0)
        result = 1;
    return result;
}

int main(int argc, char **argv)
{
    puts("LumeOS installer\n");
    hardware();
    int found = disks();
    if (argc == 2 && eq(argv[1], "--list"))
        return 0;
    if (argc > 2)
    {
        puts("Usage: install [--list|/dev/DISK]\n");
        return 2;
    }
    if (!INSTALL_ARCH)
    {
        puts("This platform uses a board-specific installation image.\n");
        return 1;
    }
    if (!found)
    {
        puts("No block disks detected. Check the storage controller and its driver.\n");
        return 1;
    }
    if (argc == 2)
        return install(argv[1]);
    char path[LUME_PATH_MAX];
    puts("Choose the target disk by its full path, or press Enter to exit:\n> ");
    if (!line(path, sizeof(path)) || !path[0])
        return 0;
    return install(path);
}
