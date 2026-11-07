// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_LUME_CHROME_APPS_FEATURES_H_
#define CHROME_COMMON_LUME_CHROME_APPS_FEATURES_H_

#include "base/component_export.h"
#include "base/feature_list.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_LUME)
namespace features
{

COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kChromeAppsDeprecation);

} // namespace features

namespace chrome
{

inline constexpr char kChromeAppsDeprecationLearnMoreURL[] =
    "https://support.google.com/chrome?p=chrome_app_deprecation";

} // namespace chrome
#endif

#endif // CHROME_COMMON_LUME_CHROME_APPS_FEATURES_H_
