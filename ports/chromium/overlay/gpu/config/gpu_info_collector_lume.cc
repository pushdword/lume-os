// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/config/gpu_info_collector.h"

#include <fcntl.h>
#include <unistd.h>
#include "lume/abi.h"
#include "lume/render.h"
#include "lume/syscall_native.h"

#include "base/check.h"
#include "base/trace_event/trace_event.h"
#include "ui/gl/gl_display.h"
#include "ui/gl/gl_implementation.h"
#include "ui/gl/gl_utils.h"

namespace gpu
{

bool CollectBasicGraphicsInfo(GPUInfo *gpu_info)
{
    DCHECK(gpu_info);
    int fd = open("/dev/dri/renderD128", O_RDWR | O_CLOEXEC);
    if (fd < 0)
        return false;
    struct lume_render_pci pci = {};
    struct lume_render_request request = {.version = LUME_RENDER_VERSION,
                                          .bytes = sizeof(request),
                                          .fd = fd,
                                          .data = reinterpret_cast<uintptr_t>(&pci),
                                          .length = sizeof(pci)};
    int64_t result = lume_native_call(LUME_SYS_GPU, LUME_RENDER_PCI,
                                      reinterpret_cast<uintptr_t>(&request), sizeof(request));
    close(fd);
    if (result || !(pci.vendor_product & 65535u) || (pci.vendor_product & 65535u) == 65535u)
        return false;
    gpu_info->gpu.vendor_id = pci.vendor_product & 65535u;
    gpu_info->gpu.device_id = pci.vendor_product >> 16;
    gpu_info->gpu.active = true;
    return true;
}

bool CollectContextGraphicsInfo(GPUInfo *gpu_info)
{
    DCHECK(gpu_info);
    TRACE_EVENT0("gpu", "gpu_info_collector::CollectGraphicsInfo");
    const auto implementation = gl::GetGLImplementation();
    if (implementation == gl::kGLImplementationNone ||
        implementation == gl::kGLImplementationDisabled)
    {
        return false;
    }
    gl::GLDisplay *display = gl::GetDefaultDisplay();
    if (!display)
    {
        return false;
    }
    return CollectGraphicsInfoGL(gpu_info, display);
}

} // namespace gpu
