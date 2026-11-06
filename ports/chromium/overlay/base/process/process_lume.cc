// SPDX-License-Identifier: BSD-3-Clause
#include "base/process/process.h"

namespace base
{

Time Process::CreationTime() const
{
    // Process creation timestamps are not available through the process query ABI.
    return Time();
}

bool Process::CanSetPriority()
{
    return false;
}

Process::Priority Process::GetPriority() const
{
    // Lume has one scheduling priority. This is also the API's failure fallback.
    return Priority::kUserBlocking;
}

bool Process::SetPriority(Priority)
{
    return false;
}

} // namespace base
