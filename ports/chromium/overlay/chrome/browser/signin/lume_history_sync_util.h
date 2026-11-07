// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SIGNIN_LUME_HISTORY_SYNC_UTIL_H_
#define CHROME_BROWSER_SIGNIN_LUME_HISTORY_SYNC_UTIL_H_

#include "build/build_config.h"

#if BUILDFLAG(IS_LUME)
namespace signin
{
class IdentityManager;
}

namespace syncer
{
class SyncService;
}

namespace signin_util
{
bool HasExplicitlyDisabledHistorySync(const syncer::SyncService *sync_service,
                                      const signin::IdentityManager *identity_manager);
}
#endif

#endif
