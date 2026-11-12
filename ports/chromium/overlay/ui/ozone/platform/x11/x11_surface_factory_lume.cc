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
#if LUME_NATIVE_EGL
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_surface_egl_x11_gles2.h"
#include "ui/ozone/common/gl_ozone_egl.h"

extern "C" gl::GLFunctionPointerType
LumeEGLGetProcAddress(const char *) __asm__("eglGetProcAddress");
#endif

namespace ui
{
#if LUME_NATIVE_EGL
namespace
{
class GLOzoneEGLNative : public GLOzoneEGL
{
  public:
    scoped_refptr<gl::GLSurface> CreateViewGLSurface(gl::GLDisplay *display,
                                                     gfx::AcceleratedWidget window) override
    {
        return gl::InitializeGLSurface(base::MakeRefCounted<gl::NativeViewGLSurfaceEGLX11GLES2>(
            display->GetAs<gl::GLDisplayEGL>(), static_cast<x11::Window>(window)));
    }

    scoped_refptr<gl::GLSurface> CreateOffscreenGLSurface(gl::GLDisplay *display,
                                                          const gfx::Size &size) override
    {
        auto *egl = display->GetAs<gl::GLDisplayEGL>();
        if (egl->IsEGLSurfacelessContextSupported() && size.IsEmpty())
            return gl::InitializeGLSurface(base::MakeRefCounted<gl::SurfacelessEGL>(egl, size));
        return gl::InitializeGLSurface(base::MakeRefCounted<gl::PbufferGLSurfaceEGL>(egl, size));
    }

  protected:
    gl::EGLDisplayPlatform GetNativeDisplay() override
    {
        return gl::EGLDisplayPlatform(reinterpret_cast<EGLNativeDisplayType>(
            x11::Connection::Get()->GetXlibDisplay().display()));
    }

    bool LoadGLES2Bindings(const gl::GLImplementationParts &implementation) override
    {
        if (implementation.gl != gl::kGLImplementationEGLGLES2)
            return false;
        gl::SetGLGetProcAddressProc(LumeEGLGetProcAddress);
        return true;
    }
};
} // namespace
#endif

X11SurfaceFactory::X11SurfaceFactory(std::unique_ptr<x11::Connection> connection)
    : connection_(std::move(connection))
{
}

X11SurfaceFactory::~X11SurfaceFactory() = default;

std::vector<gl::GLImplementationParts> X11SurfaceFactory::GetAllowedGLImplementations()
{
#if LUME_NATIVE_EGL
    return {gl::GLImplementationParts(gl::kGLImplementationEGLGLES2)};
#else
    return {gl::GLImplementationParts(gl::kGLImplementationDisabled)};
#endif
}

GLOzone *X11SurfaceFactory::GetGLOzone(const gl::GLImplementationParts &implementation)
{
#if LUME_NATIVE_EGL
    if (implementation.gl == gl::kGLImplementationEGLGLES2)
    {
        if (!egl_implementation_)
            egl_implementation_ = std::make_unique<GLOzoneEGLNative>();
        return egl_implementation_.get();
    }
#endif
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
