#define LUME_PKG_HOST_TEST 1
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <windows.h>
#else
#include <signal.h>
#endif

static bool boot_staging;

static void test_exit(unsigned status)
{
#ifdef _WIN32
    ExitProcess(status);
#else
    _Exit((int)status);
#endif
}

static bool boot_staging;

static int test_rename(const char *from, const char *to)
{
    int result = rename(from, to);
    static unsigned count;
    const char *stop = getenv("PKG_TEST_EXIT_AFTER_RENAME");
    if (!result && stop && ++count == (unsigned)atoi(stop))
        test_exit(97);
    return result;
}

static int test_commit(int fd)
{
#ifdef _WIN32
    int result = _commit(fd);
#else
    int result = fsync(fd);
#endif
    static unsigned count;
    const char *stop = getenv("PKG_TEST_EXIT_AFTER_SYNC");
    if (!result && stop && ++count == (unsigned)atoi(stop))
        test_exit(98);
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
    ssize_t result = read(fd, data, (unsigned)bytes);
    if (boot_staging && result > 0 && getenv("PKG_TEST_BOOT_TAMPER"))
        ((unsigned char *)data)[0] ^= 1;
    return result;
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
#ifndef _WIN32
#define fsync test_commit
#endif
#define read test_read
#define write test_write
#ifdef _WIN32
#define lstat stat
#endif
#define main pkg_main
#include "../../apps/pkg/pkg.c"
#undef main

int main(int argc, char **argv)
{
#ifdef _WIN32
    _set_fmode(_O_BINARY);
#endif
    return pkg_main(argc, argv);
}

unsigned pkg_current_pid(void)
{
#ifdef _WIN32
    return (unsigned)GetCurrentProcessId();
#else
    return (unsigned)getpid();
#endif
}

unsigned pkg_current_abi(void)
{
    return PKG_ABI;
}

int pkg_owner_alive(unsigned pid)
{
#ifdef _WIN32
    HANDLE owner = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!owner)
        return GetLastError() == ERROR_INVALID_PARAMETER ? 0 : -1;
    DWORD status;
    int result = GetExitCodeProcess(owner, &status) ? status == STILL_ACTIVE : -1;
    CloseHandle(owner);
    return result;
#else
    if (!pid)
        return 0;
    int result = kill((pid_t)pid, 0);
    return result == 0 || errno == EPERM ? 1 : errno == ESRCH ? 0 : -1;
#endif
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

int pkg_payload_mode(int fd, unsigned mode)
{
    (void)fd;
    (void)mode;
    return 0;
}

int pkg_boot(unsigned operation, struct lume_boot_request *request)
{
    static FILE *output;
    static unsigned total, written, checked;
    const char *trace = getenv("PKG_TEST_BOOT_TRACE");
    if (!trace)
        return -1;
    FILE *log = fopen(trace, "ab");
    if (!log)
        return -1;
    fprintf(log, "%u\n", operation);
    fclose(log);
    const char *failure = getenv("PKG_TEST_BOOT_FAILURE");
    if (failure && operation == (unsigned)atoi(failure))
        return -1;
    if (operation == LUME_BOOT_CAPABILITIES)
    {
        request->total = LUME_BOOT_CAP_RESTART_GUARD |
                         (getenv("PKG_TEST_BOOT_NO_TRIAL") ? 0 : LUME_BOOT_CAP_TRIAL);
        return 0;
    }
    if (operation == LUME_BOOT_INFO)
    {
        request->total = getenv("PKG_TEST_BOOT_TRIAL") ? LUME_BOOT_STATE_TRIAL : 0;
        return 0;
    }
    if (operation == LUME_BOOT_CONFIRM)
    {
        request->active = 1;
        return 0;
    }
    if (operation == LUME_BOOT_BEGIN)
    {
        const char *path = getenv("PKG_TEST_BOOT_OUTPUT");
        if (!path || output || request->total < 1048576 || (request->total & 511))
            return -1;
        output = fopen(path, "w+b");
        if (!output)
            return -1;
        boot_staging = true;
        total = request->total;
        written = checked = 0;
        request->active = 0;
        return 0;
    }
    if (operation == LUME_BOOT_WRITE)
    {
        if (!output || request->offset != written || request->count > total - written ||
            fwrite(request->data, 1, request->count, output) != request->count)
            return -1;
        written += request->count;
        request->offset = written;
        return (int)request->count;
    }
    if (operation == LUME_BOOT_VERIFY)
    {
        boot_staging = false;
        if (!output || written != total || fflush(output))
            return -1;
        if (getenv("PKG_TEST_BOOT_STALL"))
        {
            request->offset = 0;
            return 1;
        }
        unsigned count = total - checked;
        if (count > sizeof(request->data))
            count = sizeof(request->data);
        checked += count;
        request->offset = checked;
        return checked == total ? 0 : 1;
    }
    if (operation == LUME_BOOT_ACTIVATE)
    {
        if (!output || checked != total || fclose(output))
            return -1;
        output = NULL;
        request->active = 1;
        return 0;
    }
    if (operation == LUME_BOOT_ABORT)
    {
        boot_staging = false;
        if (output)
            fclose(output);
        output = NULL;
        return 0;
    }
    return -1;
}
