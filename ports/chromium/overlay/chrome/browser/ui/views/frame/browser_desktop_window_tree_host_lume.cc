// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/browser_desktop_window_tree_host.h"

#include "chrome/browser/ui/views/frame/browser_widget.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host_platform.h"

namespace
{

class BrowserDesktopWindowTreeHostLume final : public BrowserDesktopWindowTreeHost,
                                               public views::DesktopWindowTreeHostPlatform
{
  public:
    BrowserDesktopWindowTreeHostLume(views::internal::NativeWidgetDelegate *native_widget_delegate,
                                     views::DesktopNativeWidgetAura *desktop_native_widget_aura,
                                     BrowserWidget *browser_widget)
        : DesktopWindowTreeHostPlatform(native_widget_delegate, desktop_native_widget_aura)
    {
        browser_widget->set_frame_type(views::Widget::FrameType::kForceCustom);
    }

    ~BrowserDesktopWindowTreeHostLume() override = default;

    views::DesktopWindowTreeHost *AsDesktopWindowTreeHost() override
    {
        return this;
    }

    bool UsesNativeSystemMenu() const override
    {
        return false;
    }
};

} // namespace

BrowserDesktopWindowTreeHost *BrowserDesktopWindowTreeHost::CreateBrowserDesktopWindowTreeHost(
    views::internal::NativeWidgetDelegate *native_widget_delegate,
    views::DesktopNativeWidgetAura *desktop_native_widget_aura, BrowserView *,
    BrowserWidget *browser_widget)
{
    return new BrowserDesktopWindowTreeHostLume(native_widget_delegate, desktop_native_widget_aura,
                                                browser_widget);
}
