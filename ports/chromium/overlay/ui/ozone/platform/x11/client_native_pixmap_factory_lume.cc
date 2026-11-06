// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/x11/client_native_pixmap_factory_x11.h"

#include <memory>

#include "ui/gfx/client_native_pixmap_factory.h"
#include "ui/gfx/native_pixmap_handle.h"

namespace ui
{
namespace
{

class X11ClientNativePixmapFactory : public gfx::ClientNativePixmapFactory
{
  public:
    std::unique_ptr<gfx::ClientNativePixmap> ImportFromHandle(gfx::NativePixmapHandle,
                                                              const gfx::Size &,
                                                              viz::SharedImageFormat,
                                                              gfx::BufferUsage) override
    {
        return nullptr;
    }
};

} // namespace

gfx::ClientNativePixmapFactory *CreateClientNativePixmapFactoryX11()
{
    return new X11ClientNativePixmapFactory;
}

} // namespace ui
