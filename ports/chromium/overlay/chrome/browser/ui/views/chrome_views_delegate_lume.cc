// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/chrome_views_delegate.h"

#include "base/feature_list.h"
#include "chrome/browser/ui/views/native_widget_factory.h"
#include "ui/base/ui_base_features.h"
#include "ui/ozone/public/ozone_platform.h"
#include "ui/views/widget/widget_delegate.h"

namespace
{

NativeWidgetType GetNativeWidgetTypeForInitParams(const views::Widget::InitParams &params)
{
    // If this is a security surface, always use a toplevel window,
    // otherwise it's possible for things like menus to obscure the view.
    if (params.z_order && params.z_order.value() == ui::ZOrderLevel::kSecuritySurface)
        return NativeWidgetType::kDesktopNativeWidgetAura;

    const bool default_desktop_bubble =
        (params.type == views::Widget::InitParams::TYPE_BUBBLE ||
         params.type == views::Widget::InitParams::TYPE_POPUP) &&
        base::FeatureList::IsEnabled(features::kOzoneBubblesUsePlatformWidgets) &&
        ui::OzonePlatform::GetInstance()
            ->GetPlatformRuntimeProperties()
            .supports_subwindows_as_accelerated_widgets;

    if (!params.child && params.use_accelerated_widget_override.value_or(default_desktop_bubble))
        return NativeWidgetType::kDesktopNativeWidgetAura;
    if (params.delegate && params.delegate->use_desktop_widget_override())
        return NativeWidgetType::kDesktopNativeWidgetAura;

    return params.parent && params.type != views::Widget::InitParams::TYPE_MENU &&
                   params.type != views::Widget::InitParams::TYPE_TOOLTIP
               ? NativeWidgetType::kNativeWidgetAura
               : NativeWidgetType::kDesktopNativeWidgetAura;
}

} // namespace

views::NativeWidget *
ChromeViewsDelegate::CreateNativeWidget(views::Widget::InitParams *params,
                                        views::internal::NativeWidgetDelegate *delegate)
{
    return ::CreateNativeWidget(GetNativeWidgetTypeForInitParams(*params), params, delegate);
}
