// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_LUME_WEB_APP_SETTINGS_CONSTANTS_H_
#define CHROME_COMMON_LUME_WEB_APP_SETTINGS_CONSTANTS_H_

#include "build/build_config.h"

#if BUILDFLAG(IS_LUME)
namespace chrome
{
inline constexpr char kChromeUIWebAppSettingsHost[] = "app-settings";
inline constexpr char kChromeUIWebAppSettingsURL[] = "chrome://app-settings/";
}
#endif

#endif
