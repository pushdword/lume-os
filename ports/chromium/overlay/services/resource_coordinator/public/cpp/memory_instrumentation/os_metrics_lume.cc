// SPDX-License-Identifier: BSD-3-Clause
#include "services/resource_coordinator/public/cpp/memory_instrumentation/os_metrics.h"

namespace memory_instrumentation
{

bool OSMetrics::FillOSMemoryDump(base::ProcessHandle, const MemDumpFlagSet &, mojom::RawOSMemDump *)
{
    // The memory ABI does not expose a complete process private footprint.
    return false;
}

std::vector<mojom::VmRegionPtr> OSMetrics::GetProcessMemoryMaps(base::ProcessHandle)
{
    return {};
}

} // namespace memory_instrumentation
