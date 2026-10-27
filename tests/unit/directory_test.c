#define _GNU_SOURCE
#define _DIRENT_H
#define _DIRENT_H_
#include "../../include/posix/sys/dirent.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned allocations;
static bool allocation_failure;

static void *directory_calloc(size_t count, size_t bytes)
{
    if (allocation_failure)
    {
        errno = ENOMEM;
        return NULL;
    }
    void *p = calloc(count, bytes);
    if (p)
        ++allocations;
    return p;
}

static void directory_free(void *p)
{
    assert(p && allocations);
    --allocations;
    free(p);
}

#define calloc directory_calloc
#define free directory_free
#define opendir directory_opendir
#define readdir directory_readdir
#define closedir directory_closedir
#define telldir directory_telldir
#define seekdir directory_seekdir
#define rewinddir directory_rewinddir
#include "../../libc/posix/directory.c"
#undef calloc
#undef free

static struct
{
    uint64_t inode;
    uint32_t kind;
    char name[48];
    bool linked;
} entries[41];

static unsigned entry_count, opens, stats, closes, fetches, seeks;
static uint64_t descriptor_cookie;
static bool descriptor_live;
static unsigned descriptor_flags;
static const char *expected_path;
static int open_error, stat_error, close_error, fetch_error, seek_error;
static int malformed_count;
static bool malformed_name, malformed_cookie;
static unsigned metadata_kind;

int64_t lume_abi_host_call(unsigned number, uint64_t a, uint64_t b, uint64_t c)
{
    if (number == LUME_SYS_FILE_MODE)
    {
        assert(a == LUME_FILE_MODE_OPEN && c == sizeof(struct lume_file_mode_request));
        const struct lume_file_mode_request *r = (const void *)(uintptr_t)b;
        assert(r->version == LUME_FILE_MODE_VERSION && r->bytes == sizeof(*r));
        assert(r->path == (uintptr_t)expected_path && !r->flags);
        assert(r->descriptor_flags == LUME_FD_CLOEXEC);
        ++opens;
        if (open_error)
            return open_error;
        assert(!descriptor_live);
        descriptor_live = true;
        descriptor_cookie = 0;
        descriptor_flags = r->descriptor_flags;
        return 9;
    }
    assert(a == 9 && descriptor_live);
    if (number == LUME_SYS_STAT)
    {
        assert(!c);
        ++stats;
        if (stat_error)
            return stat_error;
        *(struct lume_native_stat *)(uintptr_t)b =
            (struct lume_native_stat){.inode = 1, .kind = metadata_kind};
        return 0;
    }
    if (number == LUME_SYS_CLOSE)
    {
        assert(!b && !c);
        ++closes;
        descriptor_live = false;
        return close_error;
    }
    if (number == LUME_SYS_SEEK)
    {
        assert(!c);
        ++seeks;
        if (seek_error)
            return seek_error;
        descriptor_cookie = b;
        return (int64_t)b;
    }
    assert(number == LUME_SYS_GETDENTS && c == sizeof(((DIR *)0)->buffer));
    ++fetches;
    if (fetch_error)
        return fetch_error;
    if (malformed_count)
        return malformed_count;
    uint64_t used = 0;
    for (unsigned i = 0; i < entry_count && c - used >= sizeof(struct lume_dirent); ++i)
    {
        if (!entries[i].linked || entries[i].inode <= descriptor_cookie)
            continue;
        struct lume_dirent entry = {.inode = entries[i].inode, .kind = entries[i].kind};
        memcpy(entry.name, entries[i].name, sizeof(entry.name));
        if (malformed_name)
            memset(entry.name, 'x', sizeof(entry.name));
        if (malformed_cookie)
            entry.inode = 0;
        memcpy((unsigned char *)(uintptr_t)b + used, &entry, sizeof(entry));
        used += sizeof(entry);
        descriptor_cookie = entries[i].inode;
    }
    return (int64_t)used;
}

int lume_errno_from_syscall(unsigned error)
{
    switch (error)
    {
    case LUME_EACCES:
        return EACCES;
    case LUME_EBADF:
        return EBADF;
    case LUME_EIO:
        return EIO;
    case LUME_EAGAIN:
        return EAGAIN;
    default:
        assert(0);
        return EIO;
    }
}

static void reset(void)
{
    assert(!allocations && !descriptor_live);
    opens = stats = closes = fetches = seeks = 0;
    open_error = stat_error = close_error = fetch_error = seek_error = malformed_count = 0;
    malformed_name = malformed_cookie = allocation_failure = false;
    metadata_kind = LUME_NODE_DIR;
    expected_path = "relative-directory";
    entry_count = 40;
    for (unsigned i = 0; i < entry_count; ++i)
    {
        entries[i].inode = 5 + i * 3;
        entries[i].kind = i % 7 + 1;
        entries[i].linked = true;
        snprintf(entries[i].name, sizeof(entries[i].name), "entry-%u", i);
    }
    memset(entries[3].name, 'n', sizeof(entries[3].name) - 1);
    entries[3].name[sizeof(entries[3].name) - 1] = 0;
}

static void enumeration(void)
{
    reset();
    DIR *directory = opendir(expected_path);
    assert(directory && opens == 1 && stats == 1 && !fetches && !closes);
    assert(descriptor_flags == LUME_FD_CLOEXEC && !telldir(directory));
    struct dirent *entry = readdir(directory);
    assert(entry && entry->d_ino == entries[0].inode && fetches == 1);
    long first = telldir(directory);
    assert(first == (long)entries[0].inode && descriptor_cookie == entries[15].inode);
    entry = readdir(directory);
    assert(entry && entry->d_ino == entries[1].inode && fetches == 1);
    seekdir(directory, first);
    assert(seeks == 1 && telldir(directory) == first);
    entry = readdir(directory);
    assert(entry && entry->d_ino == entries[1].inode && fetches == 2);

    expected_path = "another-working-directory";
    strcpy(entries[20].name, "renamed-entry");
    entries[40].inode = 200;
    entries[40].kind = LUME_NODE_SOCKET;
    entries[40].linked = true;
    strcpy(entries[40].name, "new-entry");
    entry_count = 41;
    rewinddir(directory);
    assert(!telldir(directory));
    fetches = 0;
    static const unsigned types[] = {DT_UNKNOWN, DT_REG, DT_DIR,  DT_CHR,
                                     DT_FIFO,    DT_BLK, DT_SOCK, DT_UNKNOWN};
    for (unsigned i = 0; i < entry_count; ++i)
    {
        entry = readdir(directory);
        assert(entry && entry->d_ino == entries[i].inode);
        assert(entry->d_reclen == sizeof(*entry) && entry->d_type == types[entries[i].kind]);
        assert(!strcmp(entry->d_name, entries[i].name));
        assert(telldir(directory) == (long)entries[i].inode);
    }
    errno = ERANGE;
    assert(!readdir(directory) && errno == ERANGE && fetches == 4);
    assert(!readdir(directory) && errno == ERANGE && fetches == 4);
    assert(opens == 1 && stats == 1);

    entries[3].linked = false;
    rewinddir(directory);
    for (unsigned i = 0; i < entry_count; ++i)
        if (entries[i].linked)
        {
            entry = readdir(directory);
            assert(entry && entry->d_ino == entries[i].inode);
        }
    assert(!readdir(directory));
    assert(!closedir(directory) && closes == 1 && !allocations && !descriptor_live);
}

static void failures(void)
{
    reset();
    allocation_failure = true;
    assert(!opendir(expected_path) && errno == ENOMEM && !opens && !allocations);
    allocation_failure = false;
    open_error = -LUME_EACCES;
    assert(!opendir(expected_path) && errno == EACCES && !stats && !closes && !allocations);
    open_error = 0;
    stat_error = -LUME_EACCES;
    close_error = -LUME_EIO;
    assert(!opendir(expected_path) && errno == EACCES && closes == 1 && !allocations);
    stat_error = 0;
    metadata_kind = LUME_NODE_FILE;
    assert(!opendir(expected_path) && errno == ENOTDIR && closes == 2 && !allocations);
    metadata_kind = LUME_NODE_DIR;
    close_error = 0;
    DIR *directory = opendir(expected_path);
    assert(directory);
    fetch_error = -LUME_EAGAIN;
    assert(!readdir(directory) && errno == EAGAIN && !telldir(directory));
    fetch_error = 0;
    malformed_count = 1;
    assert(!readdir(directory) && errno == EIO);
    malformed_count = sizeof(directory->buffer) + sizeof(struct lume_dirent);
    assert(!readdir(directory) && errno == EIO);
    malformed_count = 0;
    malformed_name = true;
    assert(!readdir(directory) && errno == EIO);
    malformed_name = false;
    rewinddir(directory);
    malformed_cookie = true;
    assert(!readdir(directory) && errno == EIO);
    malformed_cookie = false;
    rewinddir(directory);
    struct dirent *entry = readdir(directory);
    assert(entry && entry->d_ino == entries[0].inode);
    long first = telldir(directory);
    unsigned fetched = fetches;
    seek_error = -LUME_EIO;
    seekdir(directory, 0);
    assert(errno == EIO && telldir(directory) == first);
    entry = readdir(directory);
    assert(entry && entry->d_ino == entries[1].inode && fetches == fetched);
    unsigned sought = seeks;
    seekdir(directory, -1);
    assert(errno == EINVAL && seeks == sought);
    close_error = -LUME_EIO;
    assert(closedir(directory) == -1 && errno == EIO && !allocations && !descriptor_live);
    assert(!readdir(NULL) && errno == EBADF);
    assert(closedir(NULL) == -1 && errno == EBADF);
    assert(telldir(NULL) == -1 && errno == EBADF);
    seekdir(NULL, 0);
    assert(errno == EBADF);
}

int main(void)
{
    enumeration();
    failures();
    puts("PASS_POSIX_DIRECTORY batches, cookies, mutations, types, errors and descriptor lifetime");
    return 0;
}
