// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/fullscreen.h"

#include "ui/views/widget/desktop_aura/desktop_window_tree_host_platform.h"
#include "ui/views/widget/widget.h"

bool IsFullScreenMode()
{
    const auto windows = views::DesktopWindowTreeHostPlatform::GetAllOpenWindows();
    if (windows.empty())
        return false;
    const views::Widget *widget = views::Widget::GetWidgetForNativeWindow(windows.front());
    return widget && widget->IsFullscreen();
}
