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
#include "perf.h"
#include "fd.h"
#include <stddef.h>
#include <string.h>

static int64_t native(unsigned number, uint64_t a, uint64_t b, uint64_t c)
{
#ifdef LUME_USER_HOST_TEST
    extern int64_t lume_abi_host_call(unsigned, uint64_t, uint64_t, uint64_t);
    return lume_abi_host_call(number, a, b, c);
#else
    register uint64_t x8 __asm__("x8") = number, x0 __asm__("x0") = a, x1 __asm__("x1") = b,
                         x2 __asm__("x2") = c;
    __asm__ volatile("svc #0x4c55" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2) : "memory", "cc");
    return (int64_t)x0;
#endif
}

static int valid(uint32_t p, size_t bytes, unsigned write)
{
    return (int)native(LUME_SYS_PROBE, p, bytes, write);
}

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

static int metadata(uint32_t source, uint32_t output, unsigned fd)
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

static int spawn(uint32_t path, unsigned argc, uint32_t argv, unsigned envc, uint32_t envp,
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

int32_t lume_syscall(uint32_t number, uint32_t a, uint32_t b, uint32_t c)
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
        return 0;
    case LUME_INFO:
        if (a == LUME_UPTIME)
            return (int32_t)native(LUME_SYS_CLOCK, 0, 0, 0);
        if (a == LUME_CORES)
            return 1;
        if (a == LUME_QUANTUM_MS)
            return 250;
        if (a == LUME_PERF_STATUS)
            return 0x101;
        return a == LUME_CORE || a == LUME_FORMS ? 0 : -22;
    case LUME_PS:
        return (int32_t)native(LUME_SYS_PROCESS, a, b, sizeof(struct lume_task_info));
    case LUME_SPAWN:
        return spawn(a, b, c, 0, 0, 0);
    case LUME_SPAWN_ACTIONS:
        if (b != sizeof(struct lume_spawn_actions_request) || c)
            return -22;
        if (valid(a, b, 0))
            return -14;
        return (int32_t)native(LUME_SYS_RUN_ACTIONS, a, 0, 0);
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
            value = native(LUME_SYS_WAIT, number == LUME_WAIT || a == UINT32_MAX ? 0 : a, 0, 0);
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
    {
        struct lume_native_stat stat;
        int64_t r = native(LUME_SYS_STAT, a, (uintptr_t)&stat, 0);
        return r < 0
                   ? (int32_t)r
                   : (int32_t)native(number == LUME_READ ? LUME_SYS_READ : LUME_SYS_WRITE, a, b, c);
    }
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
        return (int32_t)native(LUME_SYS_FS, LUME_FS_RENAME, a, b);
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
        return (int32_t)native(LUME_SYS_TERMINAL, a, b, c);
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
        if (timeout > 60000 && timeout != UINT32_MAX)
            return -22;
        struct lume_fd_request poll = *request;
        poll.timeout_ms = 0;
        uint64_t started = (uint64_t)native(LUME_SYS_CLOCK, 0, 0, 0);
        for (;;)
        {
            int result = (int)native(LUME_SYS_FD, a, (uintptr_t)&poll, sizeof(poll));
            if (result || !timeout ||
                (timeout != UINT32_MAX &&
                 (uint64_t)native(LUME_SYS_CLOCK, 0, 0, 0) - started >= timeout))
                return result;
            (void)native(LUME_SYS_SLEEP, 1, 0, 0);
        }
    }
    case LUME_BRK:
        return (int32_t)native(LUME_SYS_BRK, a, 0, 0);
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
