// SPDX-License-Identifier: BSD-3-Clause
#include "base/threading/platform_thread_metrics.h"

namespace base
{

std::optional<TimeDelta> PlatformThreadMetrics::GetCumulativeCPUUsage()
{
    // Thread CPU time is not exposed by the current scheduler query interface.
    return std::nullopt;
}

} // namespace base
