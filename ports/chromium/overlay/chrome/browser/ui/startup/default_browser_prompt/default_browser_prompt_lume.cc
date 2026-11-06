// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/startup/default_browser_prompt/default_browser_prompt.h"

#include <string>
#include <utility>

#include "base/functional/callback.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"

void RegisterDefaultBrowserPromptPrefs(PrefRegistrySimple *registry)
{
    registry->RegisterStringPref(prefs::kBrowserSuppressDefaultBrowserPrompt, std::string());
}

void ShowDefaultBrowserPrompt(Profile *, base::OnceCallback<void(bool)> done_callback)
{
    std::move(done_callback).Run(false);
}
