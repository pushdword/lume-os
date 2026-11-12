#define LUME_STAT_IMPLEMENTATION 1
#define _LARGEFILE64_SOURCE 1
#ifndef LUME_USER_HOST_TEST
#include "lume/syscall_native.h"
#endif
#define lume_stat lume_native_stat
#define LUME_SEEK_SET LUME_NATIVE_SEEK_SET
#define LUME_SEEK_CUR LUME_NATIVE_SEEK_CUR
#define LUME_SEEK_END LUME_NATIVE_SEEK_END
#include "lume/abi.h"
#undef lume_stat
#undef LUME_SEEK_SET
#undef LUME_SEEK_CUR
#undef LUME_SEEK_END
#undef LUME_ARG_BYTES
#include "api.h"
#include "lume/process.h"
#include "lume/credential.h"
#include "lume/signal.h"
#include "lume/file_mode.h"
#include "lume/file_io.h"
#include "lume/io.h"
#include "files_client.h"
#include "perf.h"
#include "fd.h"
#include <stddef.h>
#include <string.h>

int32_t lume_aarch64_open_mode(const char *, unsigned, unsigned, unsigned);
int32_t lume_aarch64_mkdir_mode(const char *, unsigned);
int32_t lume_aarch64_posix_wait(int, int *, int);

static int64_t native(unsigned number, uint64_t a, uint64_t b, uint64_t c)
{
#ifdef LUME_USER_HOST_TEST
    extern int64_t lume_abi_host_call(unsigned, uint64_t, uint64_t, uint64_t);
    return lume_abi_host_call(number, a, b, c);
#else
    return lume_native_call(number, a, b, c);
#endif
}

int32_t lume_native_process_id(void)
{
    return (int32_t)native(LUME_SYS_GETPID, 0, 0, 0);
}

int app_boot(unsigned operation, struct lume_boot_request *request)
{
    return (int)native(LUME_SYS_BOOT, operation, (uintptr_t)request, sizeof(*request));
}

static int valid(uintptr_t p, size_t bytes, unsigned write)
{
    return (int)native(LUME_SYS_PROBE, p, bytes, write);
}

void lume_native_reap_orphans(void)
{
    if (native(LUME_SYS_GETPID, 0, 0, 0) == 1)
        while (native(LUME_SYS_WAIT, 0, LUME_WAIT_ORPHANED, 1) >= 0)
        {
        }
}

int32_t lume_aarch64_posix_wait(int pid, int *status, int nohang)
{
    int64_t result = native(LUME_SYS_WAIT, pid == -1 ? 0 : (unsigned)pid, !nohang, 1);
    if (result == -LUME_EAGAIN && nohang)
        return 0;
    if (result < 0)
        return (int32_t)result;
    if (status)
        *status = (int)(uint32_t)result;
    return (int32_t)((uint64_t)result >> 32);
}

int32_t lume_native_wait_options(int pid, int *status, unsigned options)
{
    int64_t target = pid == -1 ? 0 : pid;
    if (!pid)
    {
        struct lume_signal_request request = {.version = LUME_SIGNAL_VERSION,
                                              .bytes = sizeof(request)};
        int64_t group =
            native(LUME_SYS_SIGNAL, LUME_SIGNAL_GROUP, (uintptr_t)&request, sizeof(request));
        if (group < 0)
            return (int32_t)group;
        target = -group;
    }
    unsigned flags = (options & 1 ? 0 : LUME_WAIT_BLOCK) | (options & 2 ? LUME_WAIT_STOPPED : 0) |
                     (options & 8 ? LUME_WAIT_CONTINUED : 0);
    int64_t result = native(LUME_SYS_WAIT, (uint64_t)target, flags, 1);
    if (result == -LUME_EAGAIN && (options & 1))
        return 0;
    if (result < 0)
        return (int32_t)result;
    if (status)
        *status = (int)(uint32_t)result;
    return (int32_t)((uint64_t)result >> 32);
}

int32_t lume_aarch64_open_mode(const char *path, unsigned flags, unsigned mode,
                               unsigned descriptors)
{
    struct lume_files_request request = {.path = (uintptr_t)path,
                                         .fd = LUME_FILES_AT_CWD,
                                         .mode = mode & 0777u,
                                         .descriptor_flags = descriptors};
    request.flags = flags & 3u;
    if (flags & LUME_CREAT)
        request.flags |= LUME_O_CREAT;
    if (flags & LUME_TRUNC)
        request.flags |= LUME_O_TRUNC;
    if (flags & LUME_APPEND)
        request.flags |= LUME_O_APPEND;
    if (flags & LUME_EXCL)
        request.flags |= LUME_O_EXCL;
    request.flags |= flags & (LUME_FILES_OPEN_DIRECTORY | LUME_FILES_OPEN_NOFOLLOW);
    return (int32_t)lume_native_files_call(LUME_FILES_OPEN, &request);
}

int32_t lume_aarch64_mkdir_mode(const char *path, unsigned mode)
{
    struct lume_files_request request = {
        .path = (uintptr_t)path, .fd = LUME_FILES_AT_CWD, .mode = mode & 0777u};
    return (int32_t)lume_native_files_call(LUME_FILES_MKDIR, &request);
}

int64_t lume_native_files_call(unsigned operation, struct lume_files_request *request)
{
    request->version = LUME_FILES_VERSION;
    request->bytes = sizeof(*request);
    return native(LUME_SYS_FILES, operation, (uintptr_t)request, sizeof(*request));
}

int32_t lume_native_stat_at(int64_t directory, const char *path, unsigned flags,
                            struct stat *output)
{
    if (!output)
        return -LUME_EFAULT;
    struct lume_native_stat metadata;
    struct lume_files_request request = {.path = (uintptr_t)path,
                                         .fd = directory,
                                         .buffer = (uintptr_t)&metadata,
                                         .count = sizeof(metadata),
                                         .flags = flags};
    int64_t result = lume_native_files_call(LUME_FILES_STAT, &request);
    if (result < 0)
        return (int32_t)result;
    off_t bytes = (off_t)metadata.size;
    if (bytes < 0 || (uint64_t)bytes != metadata.size ||
        (uint64_t)(ino_t)metadata.inode != metadata.inode)
        return -LUME_EOVERFLOW;
    memset(output, 0, sizeof(*output));
    output->st_size = bytes;
    output->st_ino = (ino_t)metadata.inode;
    output->st_mode = (mode_t)result;
    output->st_nlink = 1;
    output->st_blksize = 512;
    output->st_blocks = (blkcnt_t)(metadata.size / 512 + !!(metadata.size % 512));
    return 0;
}

#if defined(_PICOLIBC__) && __LARGEFILE64_VISIBLE
int32_t lume_native_stat64_at(int64_t directory, const char *path, unsigned flags,
                              struct stat64 *output)
{
    if (!output)
        return -LUME_EFAULT;
    struct lume_native_stat metadata;
    struct lume_files_request request = {.path = (uintptr_t)path,
                                         .fd = directory,
                                         .buffer = (uintptr_t)&metadata,
                                         .count = sizeof(metadata),
                                         .flags = flags};
    int64_t result = lume_native_files_call(LUME_FILES_STAT, &request);
    if (result < 0)
        return (int32_t)result;
    if (metadata.size > INT64_MAX)
        return -LUME_EOVERFLOW;
    _Static_assert(sizeof(output->st_ino) == sizeof(metadata.inode), "64-bit inode identity");
    _Static_assert(sizeof(output->st_size) == sizeof(int64_t), "64-bit file size");
    memset(output, 0, sizeof(*output));
    output->st_size = (int64_t)metadata.size;
    output->st_ino = metadata.inode;
    output->st_mode = (mode_t)result;
    output->st_nlink = 1;
    output->st_blksize = 512;
    output->st_blocks = metadata.size / 512 + !!(metadata.size % 512);
    return 0;
}
#endif

int32_t lume_aarch64_posix_seek(int fd, int32_t offset, unsigned whence)
{
    if (whence > 2)
        return -22;
    struct lume_native_stat stat;
    int64_t status = native(LUME_SYS_STAT, (unsigned)fd, (uintptr_t)&stat, 0);
    if (status < 0)
        return (int32_t)status;
    int64_t base = whence == 0   ? 0
                   : whence == 2 ? (int64_t)stat.size
                                 : native(LUME_SYS_SEEK, (unsigned)fd, 0, 1);
    if (base < 0)
        return (int32_t)base;
    if (base > INT32_MAX || (offset > 0 && offset > INT32_MAX - base))
        return -75;
    int64_t target = base + offset;
    if (target < 0)
        return -22;
    return (int32_t)native(LUME_SYS_SEEK, (unsigned)fd, (uint64_t)target, 0);
}

int64_t lume_native_seek64(int fd, int64_t offset, unsigned whence)
{
    struct lume_file_io_request request = {.version = LUME_FILE_IO_VERSION,
                                           .bytes = sizeof(request),
                                           .fd = fd,
                                           .whence = whence,
                                           .offset = offset};
    int64_t result =
        native(LUME_SYS_FILE_IO, LUME_FILE_IO_SEEK, (uintptr_t)&request, sizeof(request));
    if (result == -LUME_ENOSYS)
        return offset < INT32_MIN || offset > INT32_MAX
                   ? -75
                   : lume_aarch64_posix_seek(fd, (int32_t)offset, whence);
    return result;
}

static int metadata(uintptr_t source, uintptr_t output, unsigned fd)
{
    if (valid(output, sizeof(struct lume_stat), 1))
        return -14;
    struct lume_native_stat s;
    int64_t mode =
        native(LUME_SYS_FS, fd ? LUME_FS_STAT_FD : LUME_FS_STAT_PATH, source, (uintptr_t)&s);
    if (mode < 0)
        return (int)mode;
    if (s.size > UINT32_MAX)
        return -75;
    struct lume_stat *out = (void *)(uintptr_t)output;
    *out = (struct lume_stat){.size = (uint32_t)s.size, .mode = (uint32_t)mode, .inode = s.inode};
    return 0;
}

static int spawn(uintptr_t path, unsigned argc, uintptr_t argv, unsigned envc, uintptr_t envp,
                 unsigned inherit)
{
    if (argc > LUME_ARGS || envc > LUME_ENVS)
        return -22;
    const char *arguments[LUME_ARGS + 1];
    if (argc && valid(argv, (size_t)argc * sizeof(char *), 0))
        return -14;
    if (!argc)
    {
        argc = 1;
        arguments[0] = (const char *)(uintptr_t)path;
    }
    else
        for (unsigned i = 0; i < argc; ++i)
            arguments[i] = ((const char *const *)(uintptr_t)argv)[i];
    arguments[argc] = 0;
    struct lume_run_request request = {
        .path = path, .argv = (uintptr_t)arguments, .envp = envp, .argc = argc, .envc = envc};
    return (int)native(inherit ? LUME_SYS_RUN_FDS : LUME_SYS_RUN_ENV, (uintptr_t)&request, 0, 0);
}

int32_t lume_native_spawn_env(const char *path, unsigned argc, const char *const *argv,
                              unsigned envc, const char *const *envp)
{
    return spawn((uintptr_t)path, argc, (uintptr_t)argv, envc, (uintptr_t)envp, 0);
}

int32_t lume_native_spawn_fds(const char *path, unsigned argc, const char *const *argv,
                              unsigned envc, const char *const *envp)
{
    return spawn((uintptr_t)path, argc, (uintptr_t)argv, envc, (uintptr_t)envp, 1);
}

int32_t lume_native_fd_call(unsigned operation, void *request, unsigned bytes)
{
    return (int32_t)native(LUME_SYS_FD, operation, (uintptr_t)request, bytes);
}

int32_t lume_native_poll(void *records, unsigned count, uint64_t timeout)
{
    struct lume_poll_request request = {.version = LUME_POLL_VERSION,
                                        .bytes = sizeof(request),
                                        .buffer = (uintptr_t)records,
                                        .timeout_ms = timeout,
                                        .count = count};
    return (int32_t)native(LUME_SYS_FD, LUME_FD_WAIT, (uintptr_t)&request, sizeof(request));
}

int32_t lume_native_compat_call(uint32_t number, uintptr_t a, uintptr_t b, uintptr_t c)
{
    switch (number)
    {
    case LUME_WRITE:
        return (int32_t)native(LUME_SYS_WRITE, 1, a, b);
    case LUME_GETCHAR:
    {
        unsigned char ch;
        int64_t n = native(LUME_SYS_READ, 0, (uintptr_t)&ch, 1);
        return n == 1 ? ch : n < 0 ? (int32_t)n : -11;
    }
    case LUME_YIELD:
        return (int32_t)native(LUME_SYS_SLEEP, 0, 0, 0);
    case LUME_SLEEP:
        return (int32_t)native(LUME_SYS_SLEEP, a, 0, 0);
    case LUME_EXIT:
        return (int32_t)native(LUME_SYS_EXIT, a, 0, 0);
    case LUME_PID:
        return (int32_t)native(LUME_SYS_GETPID, 0, 0, 0);
    case LUME_UID:
    {
        if (a > 3 || b || c)
            return -LUME_EINVAL;
        struct lume_credential_request request = {.version = LUME_CREDENTIAL_VERSION,
                                                  .bytes = sizeof(request)};
        int64_t result =
            native(LUME_SYS_CREDENTIAL, LUME_CREDENTIAL_GET, (uintptr_t)&request, sizeof(request));
        if (result == -LUME_ENOSYS)
            return 0;
        if (result < 0)
            return (int32_t)result;
        return (int32_t)(a == 0   ? request.uid
                         : a == 1 ? request.euid
                         : a == 2 ? request.gid
                                  : request.egid);
    }
    case LUME_INFO:
        if (a == LUME_UPTIME)
            return (int32_t)native(LUME_SYS_CLOCK, 0, 0, 0);
        if (a == LUME_CORES)
            return (int32_t)native(LUME_SYS_CPU, 0, 0, 0);
        if (a == LUME_CORE)
            return (int32_t)native(LUME_SYS_CPU, 1, 0, 0);
        if (a == LUME_QUANTUM_MS)
            return (int32_t)native(LUME_SYS_CPU, 2, 0, 0);
        if (a == LUME_PERF_STATUS)
            return 0x101;
        return a == LUME_CORE || a == LUME_FORMS ? 0 : -22;
    case LUME_PS:
        return (int32_t)native(LUME_SYS_PROCESS, a, b, sizeof(struct lume_task_info));
    case LUME_SPAWN:
        return spawn(a, b, c, 0, 0, 0);
    case LUME_SPAWN_ACTIONS:
        if ((b != sizeof(struct lume_spawn_actions_request) &&
             b != sizeof(struct lume_spawn_options_request)) ||
            c)
            return -22;
        if (valid(a, b, 0))
            return -14;
        if (((const struct lume_spawn_actions_request *)(uintptr_t)a)->version !=
            (b == sizeof(struct lume_spawn_options_request) ? LUME_SPAWN_OPTIONS_VERSION
                                                            : LUME_SPAWN_VERSION))
            return -22;
        return (int32_t)native(LUME_SYS_RUN_ACTIONS, a, b, 0);
    case LUME_SPAWN_ENV:
    {
        if (valid(a, sizeof(struct lume_spawn_request), 0))
            return -14;
        const struct lume_spawn_request *r = (const void *)(uintptr_t)a;
        return spawn(r->path, r->argc, r->argv, r->envc, r->envp, 0);
    }
    case LUME_WAIT:
    case LUME_WAITPID:
    {
        if (c > 1)
            return -22;
        if (b && valid(b, sizeof(struct lume_wait_status), 1))
            return -14;
        int64_t value;
        do
        {
            value =
                native(LUME_SYS_WAIT, number == LUME_WAIT || a == UINT32_MAX ? 0 : a, c ? 0 : 1, 0);
            if (value == -11 && !c)
                (void)native(LUME_SYS_SLEEP, 1, 0, 0);
        } while (value == -11 && !c);
        if (value == -11 && c)
            return 0;
        if (value < 0)
            return (int32_t)value;
        unsigned pid = (unsigned)((uint64_t)value >> 32);
        if (b)
            *(struct lume_wait_status *)(uintptr_t)b =
                (struct lume_wait_status){pid, (uint32_t)value};
        return (int32_t)pid;
    }
    case LUME_OPEN:
        return (int32_t)native(LUME_SYS_FS, LUME_FS_COMPAT_OPEN, a, b & ~LUME_DEFER_SYNC);
    case LUME_CLOSE:
        return (int32_t)native(LUME_SYS_CLOSE, a, 0, 0);
    case LUME_READ:
    case LUME_FDWRITE:
        return (int32_t)native(number == LUME_READ ? LUME_SYS_READ : LUME_SYS_WRITE, a, b, c);
    case LUME_SEEK:
    {
        struct lume_native_stat stat;
        int64_t r = native(LUME_SYS_STAT, a, (uintptr_t)&stat, 0);
        if (r < 0)
            return (int32_t)r;
        if (stat.kind == LUME_NODE_FILE)
        {
            if (c > 2 || stat.size > INT64_MAX - INT32_MAX)
                return -22;
            int64_t base = c == 0   ? 0
                           : c == 2 ? (int64_t)stat.size
                                    : native(LUME_SYS_SEEK, a, 0, 1);
            if (base < 0)
                return (int32_t)base;
            if ((uint64_t)base > stat.size)
                return -22;
            int64_t target = base + (int32_t)b;
            if (target < 0 || (uint64_t)target > stat.size)
                return -22;
            if (target > INT32_MAX)
                return -75;
            return (int32_t)native(LUME_SYS_SEEK, a, (uint64_t)target, 0);
        }
        return (int32_t)native(LUME_SYS_SEEK, a, (uint64_t)(int64_t)(int32_t)b, c);
    }
    case LUME_FSYNC:
        return (int32_t)native(LUME_SYS_FS, LUME_FS_SYNC, a, 0);
    case LUME_TRUNCATE_HERE:
        return (int32_t)native(LUME_SYS_FS, LUME_FS_TRUNCATE, a, 0);
    case LUME_STAT:
    case LUME_FSTAT:
        return metadata(a, b, number == LUME_FSTAT);
    case LUME_CHDIR:
        return (int32_t)native(LUME_SYS_CHDIR, a, 0, 0);
    case LUME_GETCWD:
        return (int32_t)native(LUME_SYS_GETCWD, a, b, 0);
    case LUME_MKDIR:
        return (int32_t)native(LUME_SYS_MKDIR, a, 0, 0);
    case LUME_UNLINK:
        return (int32_t)native(LUME_SYS_FS, LUME_FS_COMPAT_REMOVE, a, 0);
    case LUME_RMDIR:
        return (int32_t)native(LUME_SYS_FS, LUME_FS_COMPAT_REMOVE, a, 1);
    case LUME_RENAME:
        if (c & ~LUME_RENAME_NOREPLACE)
            return -22;
        return (int32_t)native(LUME_SYS_FS, c ? LUME_FS_RENAME : LUME_FS_REPLACE, a, b);
    case LUME_READDIR:
    {
        if (valid(b, sizeof(struct lume_directory_entry), 1))
            return -14;
        int64_t fd = native(LUME_SYS_OPEN, a, 0, 0);
        if (fd < 0)
            return (int32_t)fd;
        struct lume_dirent entry;
        int64_t r = native(LUME_SYS_SEEK, (uint64_t)fd, c, 0);
        if (r >= 0)
            r = native(LUME_SYS_GETDENTS, (uint64_t)fd, (uintptr_t)&entry, sizeof(entry));
        (void)native(LUME_SYS_CLOSE, (uint64_t)fd, 0, 0);
        if (r <= 0)
            return (int32_t)r;
        struct lume_native_stat stat;
        int64_t mode = native(LUME_SYS_FS, LUME_FS_STAT_INODE, entry.inode, (uintptr_t)&stat);
        if (mode < 0)
            return (int32_t)mode;
        if (stat.size > UINT32_MAX)
            return -75;
        struct lume_directory_entry *out = (void *)(uintptr_t)b;
        *out = (struct lume_directory_entry){.size = (uint32_t)stat.size, .mode = (uint32_t)mode};
        unsigned i = 0;
        for (; entry.name[i] && i + 1 < sizeof(out->name); ++i)
            out->name[i] = entry.name[i];
        return (int32_t)entry.inode;
    }
    case LUME_TERMINAL:
        if (a == 5 && b <= 60000 && !c)
        {
            struct lume_pollfd input = {.fd = 0, .events = LUME_POLLIN};
            struct lume_poll_request poll = {.version = LUME_POLL_VERSION,
                                             .bytes = sizeof(poll),
                                             .buffer = (uintptr_t)&input,
                                             .timeout_ms = b,
                                             .count = 1};
            int64_t result = native(LUME_SYS_FD, LUME_FD_WAIT, (uintptr_t)&poll, sizeof(poll));
            return result < 0 ? (int32_t)result : result ? 0 : -LUME_EAGAIN;
        }
        return (int32_t)native(LUME_SYS_TERMINAL, a, b, c);
    case LUME_VIDEO:
        return (int32_t)native(LUME_SYS_VIDEO, a, b, c);
    case LUME_FD:
    {
        if (c != sizeof(struct lume_fd_request))
            return -22;
        if (valid(b, c, 0) || valid(b, c, 1))
            return -14;
        struct lume_fd_request *request = (void *)(uintptr_t)b;
        if (request->version != LUME_FD_VERSION || request->bytes != c)
            return -22;
        if (a == LUME_FD_SPAWN)
        {
            if (request->flags || request->other || request->count || request->timeout_ms)
                return -22;
            if (valid(request->buffer, sizeof(struct lume_spawn_request), 0))
                return -14;
            const struct lume_spawn_request *r = (const void *)(uintptr_t)request->buffer;
            return spawn(r->path, r->argc, r->argv, r->envc, r->envp, 1);
        }
        if (a != LUME_FD_POLL)
            return (int32_t)native(LUME_SYS_FD, a, b, c);
        uint32_t timeout = request->timeout_ms;
        if (request->count > LUME_FD_LIMIT || request->flags || request->other ||
            (timeout > 60000 && timeout != UINT32_MAX))
            return -22;
        struct lume_poll_request poll = {.version = LUME_POLL_VERSION,
                                         .bytes = sizeof(poll),
                                         .buffer = request->buffer,
                                         .timeout_ms = timeout == UINT32_MAX ? UINT64_MAX : timeout,
                                         .count = request->count};
        return (int32_t)native(LUME_SYS_FD, LUME_FD_WAIT, (uintptr_t)&poll, sizeof(poll));
    }
    case LUME_BRK:
    {
        if (a > UINT32_MAX)
            return -LUME_EOVERFLOW;
        int64_t result = native(LUME_SYS_BRK, a, 0, 0);
        return result > INT32_MAX ? -LUME_EOVERFLOW : (int32_t)result;
    }
    case LUME_MEMORY:
    {
        if (b != sizeof(struct lume_memory_info) || c != LUME_MEMORY_VERSION)
            return -22;
        if (valid(a, b, 1))
            return -14;
        struct lume_memory_snapshot snapshot;
        int64_t r = native(LUME_SYS_MEMORY, (uintptr_t)&snapshot, sizeof(snapshot), 0);
        if (r < 0)
            return (int32_t)r;
        if (snapshot.total_pages > UINT32_MAX || snapshot.free_pages > UINT32_MAX ||
            snapshot.process_pages > UINT32_MAX || snapshot.heap_base > UINT32_MAX ||
            snapshot.heap_end > UINT32_MAX || snapshot.heap_limit > UINT32_MAX)
            return -LUME_EOVERFLOW;
        *(struct lume_memory_info *)(uintptr_t)a =
            (struct lume_memory_info){LUME_MEMORY_VERSION,
                                      sizeof(struct lume_memory_info),
                                      4096,
                                      (uint32_t)snapshot.total_pages,
                                      (uint32_t)snapshot.free_pages,
                                      (uint32_t)snapshot.process_pages,
                                      (uint32_t)snapshot.heap_base,
                                      (uint32_t)snapshot.heap_end,
                                      (uint32_t)snapshot.heap_limit,
                                      0x40020000,
                                      0x40024000};
        return 0;
    }
    case LUME_PERF:
        return (int32_t)native(LUME_SYS_TELEMETRY, a, b, c);
    default:
        return -38;
    }
}

int32_t lume_syscall(uint32_t number, uint32_t a, uint32_t b, uint32_t c)
{
    return lume_native_compat_call(number, a, b, c);
}
