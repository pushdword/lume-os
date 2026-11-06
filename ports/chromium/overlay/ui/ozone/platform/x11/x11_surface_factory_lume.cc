// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/x11/x11_surface_factory.h"

#include <memory>
#include <utility>

#include "ui/events/platform/x11/x11_event_source.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/native_pixmap.h"
#include "ui/gfx/native_pixmap_handle.h"
#include "ui/gl/gl_implementation.h"
#include "ui/ozone/platform/x11/x11_canvas_surface.h"

namespace ui
{

X11SurfaceFactory::X11SurfaceFactory(std::unique_ptr<x11::Connection> connection)
    : connection_(std::move(connection))
{
}

X11SurfaceFactory::~X11SurfaceFactory() = default;

std::vector<gl::GLImplementationParts> X11SurfaceFactory::GetAllowedGLImplementations()
{
    return {gl::GLImplementationParts(gl::kGLImplementationDisabled)};
}

GLOzone *X11SurfaceFactory::GetGLOzone(const gl::GLImplementationParts &)
{
    return nullptr;
}

std::unique_ptr<SurfaceOzoneCanvas>
X11SurfaceFactory::CreateCanvasForWidget(gfx::AcceleratedWidget widget)
{
    if (connection_)
    {
        auto *connection = connection_.get();
        x11::Connection::Set(std::move(connection_));
        connection->platform_event_source = std::make_unique<X11EventSource>(connection);
    }
    return std::make_unique<X11CanvasSurface>(widget);
}

scoped_refptr<gfx::NativePixmap>
X11SurfaceFactory::CreateNativePixmap(gfx::AcceleratedWidget, gpu::VulkanDeviceQueue *, gfx::Size,
                                      viz::SharedImageFormat, gfx::BufferUsage,
                                      std::optional<gfx::Size>)
{
    return nullptr;
}

bool X11SurfaceFactory::CanCreateNativePixmapForFormat(viz::SharedImageFormat)
{
    return false;
}

scoped_refptr<gfx::NativePixmap>
X11SurfaceFactory::CreateNativePixmapFromHandle(gfx::AcceleratedWidget, gfx::Size,
                                                viz::SharedImageFormat, gfx::NativePixmapHandle)
{
    return nullptr;
}

bool X11SurfaceFactory::IsFormatSupportedForTexturing(viz::SharedImageFormat) const
{
    return false;
}

} // namespace ui
