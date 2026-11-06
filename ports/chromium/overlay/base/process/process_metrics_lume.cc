// SPDX-License-Identifier: BSD-3-Clause
#include "base/process/process_metrics.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/resource.h>
#include <unistd.h>

#include <algorithm>

#include "base/memory/ptr_util.h"
#include "lume/abi.h"
#include "lume/process.h"
#include "lume/syscall_native.h"

namespace base
{

namespace
{
bool ReadMemory(struct lume_memory_snapshot *memory)
{
    return lume_native_call(LUME_SYS_MEMORY, reinterpret_cast<uintptr_t>(memory), sizeof(*memory),
                            0) == 0;
}
} // namespace

ProcessMetrics::ProcessMetrics(ProcessHandle process) : process_(process)
{
}

std::unique_ptr<ProcessMetrics> ProcessMetrics::CreateProcessMetrics(ProcessHandle process)
{
    return WrapUnique(new ProcessMetrics(process));
}

base::expected<TimeDelta, ProcessCPUUsageError> ProcessMetrics::GetCumulativeCPUUsage()
{
    // The telemetry ABI has wrapping counters and cannot represent a cumulative CPU lifetime.
    return base::unexpected(ProcessCPUUsageError::kSystemError);
}

base::expected<ProcessMemoryInfo, ProcessUsageError> ProcessMetrics::GetMemoryInfo() const
{
    // Heap pages exclude mapped regions, executable images and thread stacks.
    // The memory ABI does not expose total resident pages.
    return base::unexpected(ProcessUsageError::kSystemError);
}

int ProcessMetrics::GetOpenFdSoftLimit() const
{
    struct rlimit limit;
    if (process_ != getpid() || getrlimit(RLIMIT_NOFILE, &limit) != 0)
    {
        return -1;
    }
    return static_cast<int>(std::min(limit.rlim_cur, static_cast<rlim_t>(INT_MAX)));
}

int ProcessMetrics::GetOpenFdCount() const
{
    int limit = GetOpenFdSoftLimit();
    if (limit < 0)
    {
        return -1;
    }
    int count = 0;
    for (int fd = 0; fd < limit; ++fd)
    {
        if (fcntl(fd, F_GETFD) >= 0)
        {
            ++count;
        }
        else if (errno != EBADF)
        {
            return -1;
        }
    }
    return count;
}

size_t GetSystemCommitCharge()
{
    struct lume_memory_snapshot memory;
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0 || !ReadMemory(&memory) || memory.free_pages > memory.total_pages)
    {
        return 0;
    }
    return (memory.total_pages - memory.free_pages) * static_cast<uint64_t>(page_size) / 1024;
}

bool GetSystemMemoryInfo(SystemMemoryInfo *info)
{
    struct lume_memory_snapshot memory;
    long page_size = sysconf(_SC_PAGESIZE);
    if (!info || page_size <= 0 || !ReadMemory(&memory))
    {
        return false;
    }
    *info = SystemMemoryInfo();
    info->total = ByteSize(memory.total_pages * static_cast<uint64_t>(page_size));
    info->free = ByteSize(memory.free_pages * static_cast<uint64_t>(page_size));
    return true;
}

ByteSize SystemMemoryInfo::GetAvailablePhysicalMemory() const
{
    return free;
}

} // namespace base
