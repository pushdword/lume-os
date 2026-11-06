// Copyright 2017 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cmath>
#include <limits>

#include "src/base/platform/platform-posix-time.h"

#if defined(__lume__)
extern "C"
{
#include <sys/_tz_structs.h>
#include <sys/lock.h>
}
#endif

namespace v8
{
namespace base
{

const char *PosixDefaultTimezoneCache::LocalTimezone(double time)
{
    if (std::isnan(time))
        return "";
    time_t tv = static_cast<time_t>(std::floor(time / msPerSecond));
    struct tm tm;
    struct tm *t = localtime_r(&tv, &tm);
#if defined(__lume__)
    if (!t)
        return "";
    __LIBC_LOCK();
    const char *name = tzname[t->tm_isdst > 0 ? 1 : 0];
    __LIBC_UNLOCK();
    return name ? name : "";
#else
    if (!t || !t->tm_zone)
        return "";
    return t->tm_zone;
#endif
}

double PosixDefaultTimezoneCache::LocalTimeOffset(double time_ms, bool is_utc)
{
    // Preserve the old behavior for non-ICU implementation by ignoring both
    // time_ms and is_utc.
    time_t tv = time(nullptr);
    struct tm tm;
    struct tm *t = localtime_r(&tv, &tm);
#if defined(__lume__)
    if (!t)
        return std::numeric_limits<double>::quiet_NaN();
    __LIBC_LOCK();
    const long offset = -__gettzinfo()->__tzrule[0].offset;
    __LIBC_UNLOCK();
    return static_cast<double>(offset) * msPerSecond;
#else
    DCHECK_NOT_NULL(t);
    // tm_gmtoff includes any daylight savings offset, so subtract it.
    return static_cast<double>(t->tm_gmtoff * msPerSecond -
                               (t->tm_isdst > 0 ? 3600 * msPerSecond : 0));
#endif
}

} // namespace base
} // namespace v8
