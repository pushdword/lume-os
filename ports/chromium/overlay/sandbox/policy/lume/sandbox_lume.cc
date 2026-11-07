// SPDX-License-Identifier: BSD-3-Clause
#include "sandbox/policy/sandbox.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string>

#include "base/command_line.h"
#include "base/files/scoped_file.h"
#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "sandbox/policy/mojom/sandbox.mojom.h"
#include "sandbox/policy/switches.h"
#include "lume/abi.h"
#include "lume/process.h"
#include "lume/sandbox.h"
#include "lume/syscall_native.h"

namespace sandbox::policy
{

namespace
{

bool PrepareX11Authority()
{
    int64_t profile = lume_native_call(LUME_SYS_SANDBOX, LUME_SANDBOX_QUERY, 0, 0);
    if (profile == LUME_SANDBOX_GPU_SOFTWARE)
        return true;
    if (profile != LUME_SANDBOX_NONE)
        return false;
    const char *path = getenv("XAUTHORITY");
    std::string fallback;
    if (!path)
    {
        const char *home = getenv("HOME");
        if (!home || !*home)
            return false;
        fallback = std::string(home) + "/.Xauthority";
        path = fallback.c_str();
    }
    base::ScopedFD authority(HANDLE_EINTR(open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW)));
    if (!authority.is_valid())
    {
        PLOG(ERROR) << "X11 authority open";
        return false;
    }
    int64_t result = lume_native_call(LUME_SYS_SANDBOX, LUME_SANDBOX_X11_AUTHORITY,
                                      static_cast<uint64_t>(authority.get()), 0);
    if (result)
    {
        LOG(ERROR) << "X11 authority registration failed: " << result;
        return false;
    }
    return true;
}

} // namespace

bool Sandbox::Initialize(sandbox::mojom::Sandbox sandbox_type)
{
    const base::CommandLine &command_line = *base::CommandLine::ForCurrentProcess();
    if (command_line.HasSwitch(switches::kNoSandbox))
    {
        return false;
    }
    unsigned profile;
    switch (sandbox_type)
    {
    case sandbox::mojom::Sandbox::kRenderer:
        profile = LUME_SANDBOX_RENDERER;
        break;
    case sandbox::mojom::Sandbox::kService:
    case sandbox::mojom::Sandbox::kServiceWithJit:
    case sandbox::mojom::Sandbox::kUtility:
    case sandbox::mojom::Sandbox::kPrintCompositor:
    case sandbox::mojom::Sandbox::kAudio:
        profile = LUME_SANDBOX_UTILITY;
        break;
    case sandbox::mojom::Sandbox::kNetwork:
        profile = LUME_SANDBOX_NETWORK;
        break;
    case sandbox::mojom::Sandbox::kGpu:
        profile = LUME_SANDBOX_GPU_SOFTWARE;
        break;
    case sandbox::mojom::Sandbox::kNoSandbox:
        if (command_line.HasSwitch(switches::kProcessType))
        {
            return false;
        }
        profile = LUME_SANDBOX_NONE;
        break;
    default:
        return false;
    }
    if (profile == LUME_SANDBOX_GPU_SOFTWARE && !PrepareX11Authority())
    {
        return false;
    }
    struct lume_system_features features = {};
    if (lume_native_call(LUME_SYS_FEATURES, reinterpret_cast<uintptr_t>(&features),
                         sizeof(features), LUME_FEATURE_VERSION) != 0 ||
        lume_native_call(LUME_SYS_CAP_DROP, features.capabilities, 0, 0) != 0)
    {
        return false;
    }
    if (profile == LUME_SANDBOX_NONE)
    {
        return lume_native_call(LUME_SYS_SANDBOX, LUME_SANDBOX_QUERY, 0, 0) == LUME_SANDBOX_NONE;
    }
    if (lume_native_call(LUME_SYS_SANDBOX, LUME_SANDBOX_ENTER, profile, LUME_SANDBOX_VERSION) != 0)
    {
        return false;
    }
    return lume_native_call(LUME_SYS_SANDBOX, LUME_SANDBOX_QUERY, 0, 0) == profile;
}

} // namespace sandbox::policy
