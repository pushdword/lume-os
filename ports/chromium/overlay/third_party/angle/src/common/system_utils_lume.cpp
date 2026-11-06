//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "common/system_utils.h"

#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <utility>

#include "errno_map.h"
#include "lume/abi.h"
#include "lume/syscall_native.h"

namespace angle
{
namespace
{
[[noreturn]] void UnavailableMetric(const char *name)
{
    errno = ENOTSUP;
    std::fprintf(stderr, "%s is unavailable\n", name);
    std::abort();
}

uint64_t MonotonicNanoseconds()
{
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
    {
        std::perror("Unable to read the monotonic clock");
        std::abort();
    }
    return static_cast<uint64_t>(now.tv_sec) * UINT64_C(1000000000) + now.tv_nsec;
}

class UnavailablePageFaultHandler final : public PageFaultHandler
{
  public:
    explicit UnavailablePageFaultHandler(PageFaultCallback callback)
        : PageFaultHandler(std::move(callback))
    {
    }

    bool enable() override
    {
        errno = ENOTSUP;
        return false;
    }

    bool disable() override
    {
        errno = ENOTSUP;
        return false;
    }
};
} // namespace

std::string GetExecutablePath()
{
    std::array<char, LUME_PATH_MAX> path = {};
    int64_t length = lume_native_call(LUME_SYS_EXEPATH, 0, reinterpret_cast<uintptr_t>(path.data()),
                                      path.size());
    if (length < 0)
    {
        errno = lume_errno_from_syscall(static_cast<unsigned>(-length));
        return {};
    }
    if (!length || static_cast<uint64_t>(length) >= path.size())
    {
        errno = ENAMETOOLONG;
        return {};
    }
    return std::string(path.data(), static_cast<size_t>(length));
}

std::string GetExecutableDirectory()
{
    return StripFilenameFromPath(GetExecutablePath());
}

std::string GetModuleDirectory()
{
    std::string directory = GetExecutableDirectory();
    if (!directory.empty())
        directory += '/';
    return directory;
}

const char *GetSharedLibraryExtension()
{
    errno = ENOTSUP;
    return "";
}

const char *GetExecutableExtension()
{
    return "";
}

char GetPathSeparator()
{
    return '/';
}

const char *GetPathSeparatorForEnvironmentVar()
{
    return ":";
}

std::string GetRootDirectory()
{
    return "/";
}

Optional<std::string> GetCWD()
{
    std::array<char, 4096> path;
    if (!getcwd(path.data(), path.size()))
        return Optional<std::string>::Invalid();
    return std::string(path.data());
}

bool SetCWD(const char *directory)
{
    return chdir(directory) == 0;
}

bool UnsetEnvironmentVar(const char *name)
{
    return unsetenv(name) == 0;
}

bool SetEnvironmentVar(const char *name, const char *value)
{
    return setenv(name, value, 1) == 0;
}

std::string GetEnvironmentVar(const char *name)
{
    const char *value = getenv(name);
    return value ? std::string(value) : std::string();
}

void *OpenSystemLibraryWithExtensionAndGetError(const char *name, SearchType search,
                                                std::string *error)
{
    (void)name;
    (void)search;
    errno = ENOTSUP;
    if (error)
        *error = "Dynamic libraries are unavailable";
    return nullptr;
}

void *GetLibrarySymbol(void *library, const char *symbol)
{
    (void)library;
    (void)symbol;
    errno = ENOTSUP;
    return nullptr;
}

std::string GetLibraryPath(void *library)
{
    (void)library;
    errno = ENOTSUP;
    return {};
}

void CloseSystemLibrary(void *library)
{
    if (library)
        errno = ENOTSUP;
}

bool IsDirectory(const char *path)
{
    struct stat info;
    return stat(path, &info) == 0 && S_ISDIR(info.st_mode);
}

bool IsSameFileDescriptor(int first, int second)
{
    struct stat a, b;
    return fstat(first, &a) == 0 && fstat(second, &b) == 0 && a.st_dev == b.st_dev &&
           a.st_ino == b.st_ino;
}

bool IsDebuggerAttached()
{
    errno = ENOTSUP;
    return false;
}

void BreakDebugger()
{
    std::abort();
}

bool CreateDirectories(const std::string &path)
{
    size_t position = 0;
    do
    {
        position = path.find('/', position);
        std::string directory = path.substr(0, position);
        if (!directory.empty() && !IsDirectory(directory.c_str()))
        {
            if (mkdir(directory.c_str(), 0777) != 0 && errno != EEXIST)
                return false;
            if (!IsDirectory(directory.c_str()))
                return false;
        }
        if (position == std::string::npos)
            break;
        ++position;
    } while (true);
    return true;
}

void MakeForwardSlashThePathSeparator(std::string &path)
{
    (void)path;
}

Optional<std::string> GetTempDirectory()
{
    const char *directory = getenv("TMPDIR");
    if (!directory || !*directory)
        directory = "/data/tmp";
    if (!IsDirectory(directory))
        return Optional<std::string>::Invalid();
    return std::string(directory);
}

Optional<std::string> CreateTemporaryFileInDirectory(const std::string &directory)
{
    return CreateTemporaryFileInDirectoryWithExtension(directory, {});
}

Optional<std::string> CreateTemporaryFileInDirectoryWithExtension(const std::string &directory,
                                                                  const std::string &extension)
{
    if (extension.find('/') != std::string::npos || extension.find('\0') != std::string::npos)
    {
        errno = EINVAL;
        return Optional<std::string>::Invalid();
    }
    static constexpr char digits[] = "0123456789abcdef";
    for (unsigned attempt = 0; attempt < 64; ++attempt)
    {
        std::array<unsigned char, 8> random;
        if (getentropy(random.data(), random.size()) != 0)
            return Optional<std::string>::Invalid();
        std::string path = ConcatenatePath(directory, ".angle.");
        for (unsigned char byte : random)
        {
            path += digits[byte >> 4];
            path += digits[byte & 15];
        }
        path += extension;
        int fd = open(path.c_str(), O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
        if (fd >= 0)
        {
            if (close(fd) == 0)
                return path;
            int saved = errno;
            unlink(path.c_str());
            errno = saved;
            return Optional<std::string>::Invalid();
        }
        if (errno != EEXIST)
            return Optional<std::string>::Invalid();
    }
    errno = EEXIST;
    return Optional<std::string>::Invalid();
}

double GetCurrentSystemTime()
{
    return static_cast<double>(MonotonicNanoseconds()) * 1e-9;
}

uint64_t GetCurrentSystemTimeNs()
{
    return MonotonicNanoseconds();
}

double GetCurrentProcessCpuTime()
{
    UnavailableMetric("Process CPU time");
}

uint64_t GetProcessMemoryUsageKB()
{
    UnavailableMetric("Process resident memory");
}

bool ProtectMemory(uintptr_t start, size_t bytes)
{
    return mprotect(reinterpret_cast<void *>(start), bytes, PROT_READ) == 0;
}

bool UnprotectMemory(uintptr_t start, size_t bytes)
{
    return mprotect(reinterpret_cast<void *>(start), bytes, PROT_READ | PROT_WRITE) == 0;
}

size_t GetPageSize()
{
    long bytes = sysconf(_SC_PAGESIZE);
    return bytes > 0 ? static_cast<size_t>(bytes) : 0;
}

PageFaultHandler *CreatePageFaultHandler(PageFaultCallback callback)
{
    return new UnavailablePageFaultHandler(std::move(callback));
}

void SetCurrentThreadName(const char *name)
{
    std::string shortName(name ? name : "");
    shortName.resize(shortName.size() < 15 ? shortName.size() : 15);
    int error = pthread_setname_np(pthread_self(), shortName.c_str());
    if (error)
        errno = error;
}
} // namespace angle
