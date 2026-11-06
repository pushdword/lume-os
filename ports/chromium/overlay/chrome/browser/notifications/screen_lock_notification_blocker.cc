// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/screen_lock_notification_blocker.h"

#include "base/time/time.h"
#include "ui/base/idle/idle.h"

namespace
{
const int kUserStatePollingIntervalSeconds = 1;
}

ScreenLockNotificationBlocker::ScreenLockNotificationBlocker(
    message_center::MessageCenter *message_center)
    : NotificationBlocker(message_center), is_locked_(false)
{
}

ScreenLockNotificationBlocker::~ScreenLockNotificationBlocker() = default;

void ScreenLockNotificationBlocker::CheckState()
{
#if BUILDFLAG(IS_LUME)
    const bool was_available = state_available_;
    const bool was_active = screen_saver_active_;
    state_available_ = ui::QueryScreenSaverState(&screen_saver_active_);
    if (screen_saver_active_ != was_active || state_available_ != was_available)
        NotifyBlockingStateChanged();

    if (screen_saver_active_ || !state_available_)
    {
#else
    bool was_locked = is_locked_;
    is_locked_ = ui::CheckIdleStateIsLocked();
    if (is_locked_ != was_locked)
        NotifyBlockingStateChanged();

    if (is_locked_)
    {
#endif
        timer_.Start(FROM_HERE, base::Seconds(kUserStatePollingIntervalSeconds), this,
                     &ScreenLockNotificationBlocker::CheckState);
    }
}

bool ScreenLockNotificationBlocker::ShouldShowNotificationAsPopup(
    const message_center::Notification &notification) const
{
#if BUILDFLAG(IS_LUME)
    return state_available_ && !screen_saver_active_;
#else
    return !is_locked_;
#endif
}
