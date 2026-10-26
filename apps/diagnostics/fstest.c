#include "api.h"
#include "fd.h"
static unsigned char data[1537], readback[1537];

static int fail(const char *name, int r)
{
    app_error(name, r);
    return 1;
}

#define REQUIRE(condition, label)                                                                  \
    do                                                                                             \
    {                                                                                              \
        if (!(condition))                                                                          \
            return fail(label, -1);                                                                \
    } while (0)

static uint32_t p(const void *v)
{
    return (uint32_t)(uintptr_t)v;
}

int app_main(int argc, char **argv)
{
    REQUIRE(argc >= 1 && argv && argv[argc] == 0, "argv contract");
    if (argc > 1)
        REQUIRE(argc == 3 && app_equal(argv[1], "two words") && app_equal(argv[2], "three"),
                "argument contents");
    const char *path = "/tmp/fstest.txt";
    int r = lume_syscall(LUME_MKDIR, p("/tmp"), 0, 0);
    REQUIRE(r == 0 || r == -17, "mkdir");
    for (unsigned i = 0; i < sizeof data; i++)
        data[i] = (unsigned char)(i * 37u + 11u);
    int fd = app_open(path, LUME_RDWR | LUME_CREAT | LUME_TRUNC);
    REQUIRE(fd >= 3, "open/create");
    REQUIRE(app_read(fd, readback, 1) == 0, "empty EOF");
    REQUIRE(app_open(path, LUME_RDWR) == -16, "writer exclusion");
    REQUIRE(lume_syscall(LUME_UNLINK, p(path), 0, 0) == -16, "open unlink exclusion");
    REQUIRE(app_write(fd, data, sizeof data) == sizeof data, "write across sectors/clusters");
    REQUIRE(app_seek(fd, -1, LUME_SEEK_END) == 1536, "seek end");
    REQUIRE(app_read(fd, readback, 1) == 1 && readback[0] == data[1536], "last byte");
    REQUIRE(app_seek(fd, 1538, LUME_SEEK_SET) == -22, "no undefined sparse gap");
    REQUIRE(app_seek(fd, 0, LUME_SEEK_SET) == 0, "rewind");
    REQUIRE(lume_syscall(LUME_READ, (uint32_t)fd, 0xf0000000, 1) == -14, "bad user pointer");
    REQUIRE(app_read(fd, readback, sizeof readback) == sizeof data, "full read");
    for (unsigned i = 0; i < sizeof data; i++)
        REQUIRE(data[i] == readback[i], "readback bytes");
    REQUIRE(app_read(fd, readback, 1) == 0, "EOF");
    REQUIRE(app_sync(fd) == 0, "fsync");
    REQUIRE(app_close(fd) == 0, "close");
    REQUIRE(app_close(fd) == -9, "double close");
    REQUIRE(lume_syscall(LUME_SPAWN, p("/bin/cat" LUME_APP_SUFFIX ""), 1, 0xf0000000u) == -14,
            "bad argv pointer");
    REQUIRE(lume_syscall(LUME_SPAWN, p("/bin/cat" LUME_APP_SUFFIX ""), LUME_ARGS + 1, p(argv)) ==
                -22,
            "argument count bound");
    REQUIRE(app_wait(0, 0) == -10, "wait only owns children");
    fd = app_open(path, LUME_WRONLY | LUME_APPEND);
    REQUIRE(fd >= 3, "append open");
    REQUIRE(app_seek(fd, 0, LUME_SEEK_SET) == 0, "append seek");
    REQUIRE(app_write(fd, "LUME", 4) == 4, "append respects EOF");
    REQUIRE(app_close(fd) == 0, "append close");
    struct lume_stat stat;
    REQUIRE(app_stat(path, &stat) == 0 && stat.size == 1541 && stat.uid == 0 && stat.gid == 0,
            "stat metadata");
    fd = app_open(path, LUME_RDONLY);
    REQUIRE(fd >= 3, "reopen");
    REQUIRE(app_write(fd, "x", 1) == -9, "access mode");
    REQUIRE(app_seek(fd, -4, LUME_SEEK_END) == 1537 && app_read(fd, readback, 4) == 4,
            "append readback");
    REQUIRE(readback[0] == 'L' && readback[1] == 'U' && readback[2] == 'M' && readback[3] == 'E',
            "append bytes");
    REQUIRE(app_close(fd) == 0, "read close");
    unsigned fd_slots = LUME_FDS - 3;
    struct lume_fd_info fd_info;
    struct lume_fd_request fd_request = {
        LUME_FD_VERSION, sizeof fd_request, 0, 0, 0, p(&fd_info), 0, 0};
    if (lume_syscall(LUME_FD, LUME_FD_INFO, p(&fd_request), sizeof fd_request) == 0)
    {
        REQUIRE(fd_info.version == LUME_FD_VERSION && fd_info.bytes == sizeof fd_info &&
                    fd_info.open == 3 && fd_info.limit >= 3 && fd_info.limit <= LUME_FD_LIMIT,
                "FD info contract");
        fd_slots = fd_info.limit - fd_info.open;
    }
    int fds[LUME_FD_LIMIT - 3];
    for (unsigned i = 0; i < fd_slots; i++)
    {
        fds[i] = app_open(path, LUME_RDONLY);
        REQUIRE(fds[i] >= 3, "FD slots");
    }
    REQUIRE(app_open(path, LUME_RDONLY) == -24, "FD limit");
    for (unsigned i = 0; i < fd_slots; i++)
        REQUIRE(app_close(fds[i]) == 0, "FD recycle");
    REQUIRE(app_open(path, LUME_WRONLY | LUME_CREAT | LUME_EXCL) == -17, "exclusive create");
    fd = app_open(path, LUME_RDWR | LUME_TRUNC);
    REQUIRE(fd >= 3 && app_close(fd) == 0, "truncate existing");
    REQUIRE(app_stat(path, &stat) == 0 && stat.size == 0, "truncated size");
    fd = app_open(path, LUME_RDWR);
    REQUIRE(fd >= 3, "rewrite");
    REQUIRE(app_write(fd, "LUME FS PERSISTENT\n", 19) == 19, "persistent text");
    REQUIRE(lume_syscall(LUME_RENAME, p(path), p("/tmp/renamed.txt"), 0) == -16,
            "open rename exclusion");
    REQUIRE(app_close(fd) == 0, "text close");
    (void)lume_syscall(LUME_UNLINK, p("/tmp/renamed.txt"), 0, 0);
    REQUIRE(lume_syscall(LUME_RENAME, p(path), p("/tmp/renamed.txt"), 0) == 0, "rename");
    REQUIRE(lume_syscall(LUME_RENAME, p("/tmp/renamed.txt"), p(path), 0) == 0, "rename back");
    app_puts("FSTEST PASS: create/write/append/seek/EOF/sync/FDs/rename/protection\n");
    /* One read FD intentionally remains open: process exit must close it. */
    REQUIRE(app_open(path, LUME_RDONLY) >= 3, "exit cleanup");
    return 0;
}
