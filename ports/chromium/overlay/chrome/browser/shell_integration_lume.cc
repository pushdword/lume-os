// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/shell_integration.h"

namespace shell_integration
{

bool SetAsDefaultBrowser()
{
    return false;
}

bool SetAsDefaultClientForScheme(const std::string &)
{
    return false;
}

std::u16string GetApplicationNameForScheme(const GURL &)
{
    return {};
}

DefaultWebClientState GetDefaultBrowser()
{
    return UNKNOWN_DEFAULT;
}

bool IsFirefoxDefaultBrowser()
{
    return false;
}

DefaultWebClientState IsDefaultClientForScheme(const std::string &)
{
    return UNKNOWN_DEFAULT;
}

namespace internal
{

DefaultWebClientSetPermission GetPlatformSpecificDefaultWebClientSetPermission(WebClientSetMethod)
{
    return SET_DEFAULT_NOT_ALLOWED;
}

} // namespace internal
} // namespace shell_integration
