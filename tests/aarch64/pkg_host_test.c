#define LUME_PKG_HOST_TEST 1
#include <direct.h>
#include <errno.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <windows.h>

static int test_rename(const char *from, const char *to)
{
    int result = rename(from, to);
    static unsigned count;
    const char *stop = getenv("PKG_TEST_EXIT_AFTER_RENAME");
    if (!result && stop && ++count == (unsigned)atoi(stop))
        ExitProcess(97);
    return result;
}

static int test_commit(int fd)
{
    int result = _commit(fd);
    static unsigned count;
    const char *stop = getenv("PKG_TEST_EXIT_AFTER_SYNC");
    if (!result && stop && ++count == (unsigned)atoi(stop))
        ExitProcess(98);
    return result;
}

static ssize_t test_read(int fd, void *data, size_t bytes)
{
    if (bytes > 65536)
    {
        errno = EINVAL;
        return -1;
    }
    if (getenv("PKG_TEST_SHORT_IO") && bytes > 997)
        bytes = 997;
    return read(fd, data, (unsigned)bytes);
}

static ssize_t test_write(int fd, const void *data, size_t bytes)
{
    if (bytes > 65536)
    {
        errno = EINVAL;
        return -1;
    }
    if (getenv("PKG_TEST_SHORT_IO") && bytes > 997)
        bytes = 997;
    return write(fd, data, (unsigned)bytes);
}

#define rename test_rename
#define _commit test_commit
#define read test_read
#define write test_write
#define lstat stat
#define main pkg_main
#include "../../apps/common/pkg.c"
#undef main

int main(int argc, char **argv)
{
    _set_fmode(_O_BINARY);
    return pkg_main(argc, argv);
}

unsigned pkg_current_pid(void)
{
    return (unsigned)GetCurrentProcessId();
}

unsigned pkg_current_abi(void)
{
    return PKG_ABI;
}

int pkg_owner_alive(unsigned pid)
{
    HANDLE owner = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!owner)
        return GetLastError() == ERROR_INVALID_PARAMETER ? 0 : -1;
    DWORD status;
    int result = GetExitCodeProcess(owner, &status) ? status == STILL_ACTIVE : -1;
    CloseHandle(owner);
    return result;
}

int pkg_download(const char *source, const char *target, uint32_t bytes,
                 const unsigned char digest[32])
{
    (void)source;
    (void)target;
    (void)bytes;
    (void)digest;
    return -1;
}
