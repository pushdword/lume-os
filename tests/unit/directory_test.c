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
#define fdopendir directory_fdopendir
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
    uint64_t inode, position;
    uint32_t kind;
    char name[48];
    bool linked;
} entries[43];

static unsigned entry_count, opens, stats, closes, fetches, seeks;
static uint64_t descriptor_cookie;
static bool descriptor_live;
static unsigned descriptor_flags;
static const char *expected_path;
static int open_error, stat_error, close_error, fetch_error, seek_error;
static int malformed_count;
static bool malformed_name, malformed_cookie;
static unsigned metadata_kind;
static int descriptor_access;

int64_t lume_abi_host_call(unsigned number, uint64_t a, uint64_t b, uint64_t c)
{
    if (number == LUME_SYS_FD)
    {
        assert(a == LUME_FD_GETMODE && c == sizeof(struct lume_fd_request));
        const struct lume_fd_request *request = (const void *)(uintptr_t)b;
        assert(request->fd == 9 && descriptor_live);
        return descriptor_access;
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
        assert(c <= 1);
        ++seeks;
        if (seek_error)
            return seek_error;
        descriptor_cookie = c ? descriptor_cookie + b : b;
        return (int64_t)descriptor_cookie;
    }
    assert(0);
    return -LUME_EIO;
}

int64_t lume_native_files_call(unsigned operation, struct lume_files_request *request)
{
    if (operation == LUME_FILES_OPEN)
    {
        assert(request->path == (uintptr_t)expected_path && request->fd == LUME_FILES_AT_CWD);
        assert(request->flags == (LUME_O_RDONLY | LUME_FILES_OPEN_DIRECTORY));
        assert(request->descriptor_flags == LUME_FD_CLOEXEC);
        ++opens;
        if (open_error)
            return open_error;
        assert(!descriptor_live);
        descriptor_live = true;
        descriptor_cookie = 0;
        descriptor_flags = request->descriptor_flags;
        return 9;
    }
    assert(operation == LUME_FILES_GETDENTS && request->flags == LUME_FILES_GETDENTS_POSIX);
    assert(request->fd == 9 && descriptor_live && request->count == sizeof(((DIR *)0)->buffer));
    ++fetches;
    if (fetch_error)
        return fetch_error;
    if (malformed_count)
        return malformed_count;
    uint64_t used = 0;
    unsigned first = descriptor_cookie == LUME_FILES_DIR_AFTER_DOT ? 1 : descriptor_cookie ? 2 : 0;
    for (unsigned i = first;
         i < entry_count && request->count - used >= sizeof(struct lume_files_dirent_posix); ++i)
    {
        if (!entries[i].linked ||
            (i >= 2 && descriptor_cookie < LUME_FILES_DIR_AFTER_DOT &&
             entries[i].position <= descriptor_cookie))
            continue;
        struct lume_files_dirent_posix entry = {.inode = entries[i].inode,
                                                .position = entries[i].position,
                                                .kind = entries[i].kind,
                                                .name_bytes = (uint32_t)strlen(entries[i].name)};
        memcpy(entry.name, entries[i].name, entry.name_bytes + 1);
        if (malformed_name)
            memset(entry.name, 'x', sizeof(entry.name));
        if (malformed_cookie)
            entry.position = 0;
        memcpy((unsigned char *)(uintptr_t)request->buffer + used, &entry, sizeof(entry));
        used += sizeof(entry);
        descriptor_cookie = entries[i].position;
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
    entry_count = 42;
    entries[0].inode = 1;
    entries[0].position = LUME_FILES_DIR_AFTER_DOT;
    entries[0].kind = LUME_NODE_DIR;
    entries[0].linked = true;
    strcpy(entries[0].name, ".");
    entries[1].inode = 2;
    entries[1].position = LUME_FILES_DIR_AFTER_DOTDOT;
    entries[1].kind = LUME_NODE_DIR;
    entries[1].linked = true;
    strcpy(entries[1].name, "..");
    for (unsigned i = 2; i < entry_count; ++i)
    {
        entries[i].inode = 5 + i * 3;
        entries[i].position = 10 + i * 2;
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
    assert(entry && !strcmp(entry->d_name, ".") && entry->d_ino == entries[0].inode);
    long first = telldir(directory);
    assert(first == (long)entries[0].position && descriptor_cookie == entries[3].position);
    entry = readdir(directory);
    assert(entry && !strcmp(entry->d_name, "..") && entry->d_ino == entries[1].inode &&
           fetches == 1);
    seekdir(directory, first);
    assert(seeks == 1 && telldir(directory) == first);
    entry = readdir(directory);
    assert(entry && entry->d_ino == entries[1].inode && fetches == 2);

    expected_path = "another-working-directory";
    strcpy(entries[20].name, "renamed-entry");
    entries[42].inode = 200;
    entries[42].position = 300;
    entries[42].kind = LUME_NODE_SOCKET;
    entries[42].linked = true;
    strcpy(entries[42].name, "new-entry");
    entry_count = 43;
    rewinddir(directory);
    assert(!telldir(directory));
    fetches = 0;
    static const unsigned types[] = {DT_UNKNOWN, DT_REG, DT_DIR,  DT_CHR,
                                     DT_FIFO,    DT_BLK, DT_SOCK, DT_LNK};
    for (unsigned i = 0; i < entry_count; ++i)
    {
        entry = readdir(directory);
        assert(entry && entry->d_ino == entries[i].inode);
        assert(entry->d_reclen == sizeof(*entry) && entry->d_type == types[entries[i].kind]);
        assert(!strcmp(entry->d_name, entries[i].name));
        assert(telldir(directory) == (long)entries[i].position);
    }
    errno = ERANGE;
    assert(!readdir(directory) && errno == ERANGE && fetches == 12);
    assert(!readdir(directory) && errno == ERANGE && fetches == 12);
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
    malformed_count = sizeof(directory->buffer) + sizeof(struct lume_files_dirent_posix);
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

static void descriptor_directory(void)
{
    reset();
    descriptor_live = true;
    descriptor_cookie = entries[2].position;
    assert(!fdopendir(-1) && errno == EBADF && descriptor_live && !closes);
    metadata_kind = LUME_NODE_FILE;
    assert(!fdopendir(9) && errno == ENOTDIR && descriptor_live && !closes);
    metadata_kind = LUME_NODE_DIR;
    descriptor_access = LUME_O_WRONLY;
    assert(!fdopendir(9) && errno == EBADF && descriptor_live && !closes);
    descriptor_access = 0;
    allocation_failure = true;
    assert(!fdopendir(9) && errno == ENOMEM && descriptor_live && !closes);
    allocation_failure = false;
    seek_error = -LUME_EIO;
    assert(!fdopendir(9) && errno == EIO && descriptor_live && !closes);
    seek_error = 0;
    DIR *directory = fdopendir(9);
    assert(directory && dirfd(directory) == 9 && telldir(directory) == (long)entries[2].position);
    struct dirent *entry = readdir(directory);
    assert(entry && entry->d_ino == entries[3].inode);
    assert(!closedir(directory) && !descriptor_live && closes == 1 && !allocations);
}

int main(void)
{
    enumeration();
    failures();
    descriptor_directory();
    puts("PASS_POSIX_DIRECTORY batches, cookies, mutations, types, errors and descriptor lifetime");
    return 0;
}
