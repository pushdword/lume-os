// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/notification_platform_bridge.h"

std::unique_ptr<NotificationPlatformBridge> NotificationPlatformBridge::Create()
{
    // The delegator uses the Views message center without a system bridge.
    return nullptr;
}

bool NotificationPlatformBridge::CanHandleType(NotificationHandler::Type)
{
    return false;
}
