// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/idle/idle.h"

#include <cerrno>
#include <cstdint>
#include <optional>

#include "ui/base/idle/idle_internal.h"
#include "ui/gfx/x/connection.h"
#include "ui/gfx/x/future.h"
#include "ui/gfx/x/screensaver.h"

namespace ui
{
namespace
{
struct IdleSnapshot
{
    uint32_t idle_ms;
    bool saver_active;
};

std::optional<IdleSnapshot> QueryIdleSnapshot()
{
    auto *connection = x11::Connection::Get();
    if (!connection || !connection->Ready())
    {
        errno = ENOTCONN;
        return std::nullopt;
    }
    auto &screensaver = connection->screensaver();
    if (!screensaver.present())
    {
        errno = ENOTSUP;
        return std::nullopt;
    }
    auto reply = screensaver.QueryInfo(connection->default_root()).Sync();
    if (!reply)
    {
        errno = EIO;
        return std::nullopt;
    }
    bool active;
    switch (static_cast<x11::ScreenSaver::State>(reply->state))
    {
    case x11::ScreenSaver::State::Off:
    case x11::ScreenSaver::State::Disabled:
        active = false;
        break;
    case x11::ScreenSaver::State::On:
    case x11::ScreenSaver::State::Cycle:
        active = true;
        break;
    default:
        errno = EIO;
        return std::nullopt;
    }
    errno = 0;
    return IdleSnapshot{reply->ms_since_user_input, active};
}
} // namespace

int CalculateIdleTime()
{
    auto snapshot = QueryIdleSnapshot();
    return snapshot ? static_cast<int>(snapshot->idle_ms / 1000) : -1;
}

bool CheckIdleStateIsLocked()
{
    if (IdleStateForTesting().has_value())
    {
        errno = 0;
        return IdleStateForTesting().value() == IDLE_STATE_LOCKED;
    }
    // Authenticated session-lock state is unavailable.
    errno = ENOTSUP;
    return false;
}

bool QueryScreenSaverState(bool *active)
{
    if (!active)
    {
        errno = EINVAL;
        return false;
    }
    auto snapshot = QueryIdleSnapshot();
    if (!snapshot)
        return false;
    *active = snapshot->saver_active;
    return true;
}

base::CallbackListSubscription AddScreenLockCallback(base::RepeatingCallback<void(bool)>)
{
    // Authenticated session-lock callbacks are unavailable.
    errno = ENOTSUP;
    return {};
}
} // namespace ui
