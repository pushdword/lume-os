// Copyright 2011 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/first_run/upgrade_util.h"

#include "base/command_line.h"
#include "base/process/launch.h"

namespace upgrade_util
{

bool RelaunchChromeBrowserImpl(const base::CommandLine &command_line)
{
    return base::LaunchProcess(command_line, base::LaunchOptions()).IsValid();
}

} // namespace upgrade_util
