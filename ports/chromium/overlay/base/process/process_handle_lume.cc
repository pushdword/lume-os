// SPDX-License-Identifier: BSD-3-Clause
#include "base/process/process_handle.h"

#include <errno.h>
#include <limits.h>
#include <unistd.h>

#include "base/files/file_path.h"
#include "lume/abi.h"
#include "lume/process.h"
#include "lume/syscall_native.h"

namespace base
{

ProcessId GetParentProcessId(ProcessHandle process)
{
    if (process <= 0)
    {
        return -1;
    }
    struct lume_task_info info;
    if (lume_native_call(LUME_SYS_PROCESS, static_cast<uint64_t>(process - 1),
                         reinterpret_cast<uintptr_t>(&info), sizeof(info)) != 1 ||
        info.pid != static_cast<uint32_t>(process))
    {
        return -1;
    }
    return static_cast<ProcessId>(info.parent);
}

FilePath GetProcessExecutablePath(ProcessHandle process)
{
    if (process <= 0)
    {
        return FilePath();
    }
    char path[LUME_PATH_MAX];
    int64_t bytes = lume_native_call(LUME_SYS_EXEPATH, process, reinterpret_cast<uintptr_t>(path),
                                     sizeof(path));
    if (bytes <= 0 || static_cast<uint64_t>(bytes) >= sizeof(path) || path[bytes] != '\0')
    {
        return FilePath();
    }
    return FilePath(std::string(path, static_cast<size_t>(bytes)));
}

} // namespace base
