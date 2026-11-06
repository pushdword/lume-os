// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/config/gpu_info_collector.h"

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
    // The native display interface does not expose graphics-device identification.
    return false;
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
