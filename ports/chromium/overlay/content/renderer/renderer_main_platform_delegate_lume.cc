// SPDX-License-Identifier: BSD-3-Clause
#include "content/renderer/renderer_main_platform_delegate.h"

#include "sandbox/policy/mojom/sandbox.mojom.h"
#include "sandbox/policy/sandbox.h"

namespace content
{

RendererMainPlatformDelegate::RendererMainPlatformDelegate(const MainFunctionParams &)
{
}

RendererMainPlatformDelegate::~RendererMainPlatformDelegate() = default;

void RendererMainPlatformDelegate::PlatformInitialize()
{
}

void RendererMainPlatformDelegate::PlatformUninitialize()
{
}

bool RendererMainPlatformDelegate::EnableSandbox()
{
    return sandbox::policy::Sandbox::Initialize(sandbox::mojom::Sandbox::kRenderer);
}

} // namespace content
