// Copyright 2011 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/idle/idle.h"

#if BUILDFLAG(IS_LUME)
#include <cerrno>
#endif

#include "ui/base/idle/idle_internal.h"

namespace ui
{

IdleState CalculateIdleState(int idle_threshold)
{
    if (IdleStateForTesting().has_value())
        return IdleStateForTesting().value();

#if BUILDFLAG(IS_LUME)
    errno = 0;
    const bool locked = CheckIdleStateIsLocked();
    if (errno)
        return IDLE_STATE_UNKNOWN;
    if (locked)
        return IDLE_STATE_LOCKED;
    const int idle_time = CalculateIdleTime();
    if (idle_time < 0)
        return IDLE_STATE_UNKNOWN;
    if (idle_time >= idle_threshold)
#else
    if (CheckIdleStateIsLocked())
        return IDLE_STATE_LOCKED;

    if (CalculateIdleTime() >= idle_threshold)
#endif
        return IDLE_STATE_IDLE;

    return IDLE_STATE_ACTIVE;
}

} // namespace ui
