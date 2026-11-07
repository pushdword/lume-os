// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/lume_chrome_apps_features.h"

#if BUILDFLAG(IS_LUME)
namespace features
{

// Controls whether Chrome Apps are supported. See https://crbug.com/40186761.
// If the feature is disabled, Chrome Apps continue to work. If enabled, Chrome
// Apps will not launch and will be marked in the UI as deprecated.
BASE_FEATURE(kChromeAppsDeprecation, base::FEATURE_ENABLED_BY_DEFAULT);

} // namespace features
#endif
