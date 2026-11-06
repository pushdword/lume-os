// SPDX-License-Identifier: BSD-3-Clause
#include "src/base/platform/platform.h"

#include <errno.h>

#include "src/base/platform/platform-posix-time.h"
#include "lume/abi.h"
#include "lume/syscall_native.h"

extern "C"
{
    extern const char __lume_text_start[];
    extern const char __lume_text_end[];
}

namespace v8::base
{

TimezoneCache *OS::CreateTimezoneCache()
{
    return new PosixDefaultTimezoneCache();
}

std::vector<OS::SharedLibraryAddress> OS::GetSharedLibraryAddresses()
{
    char path[LUME_PATH_MAX];
    int64_t bytes =
        lume_native_call(LUME_SYS_EXEPATH, 0, reinterpret_cast<uintptr_t>(path), sizeof(path));
    uintptr_t start = reinterpret_cast<uintptr_t>(__lume_text_start);
    uintptr_t end = reinterpret_cast<uintptr_t>(__lume_text_end);
    if (bytes <= 0 || static_cast<uint64_t>(bytes) >= sizeof(path) || path[bytes] != '\0' ||
        end <= start)
    {
        return {};
    }
    return {SharedLibraryAddress(std::string(path, static_cast<size_t>(bytes)), start, end)};
}

void OS::SignalCodeMovingGC()
{
    // The kernel does not publish mmap events to a profiling stream.
}

void OS::AdjustSchedulingParams()
{
    // The scheduler exposes a single priority class.
}

std::optional<OS::MemoryRange> OS::GetFirstFreeMemoryRangeWithin(OS::Address, OS::Address, size_t,
                                                                 size_t)
{
    errno = ENOTSUP;
    return std::nullopt;
}

bool OS::RemapPages(const void *, size_t, void *, MemoryPermission)
{
    errno = ENOTSUP;
    return false;
}

} // namespace v8::base
