// SPDX-License-Identifier: BSD-3-Clause
#ifndef SERVICES_NETWORK_LUME_NETWORK_DIRECTORY_ACCESS_H_
#define SERVICES_NETWORK_LUME_NETWORK_DIRECTORY_ACCESS_H_

#include <vector>

#include "base/files/file.h"
#include "lume/abi.h"
#include "lume/sandbox.h"
#include "lume/syscall_native.h"

namespace network
{

inline bool ActivateLumeNetworkDirectories(std::vector<base::File> directories)
{
    for (const auto &directory : directories)
    {
        if (!directory.IsValid() ||
            lume_native_call(LUME_SYS_SANDBOX, LUME_SANDBOX_DIRECTORY_ACTIVATE,
                             directory.GetPlatformFile(), 0) != 0)
            return false;
    }
    return true;
}

} // namespace network

#endif
