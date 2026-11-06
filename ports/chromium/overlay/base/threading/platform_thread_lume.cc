// SPDX-License-Identifier: BSD-3-Clause
#include "base/threading/platform_thread.h"

#include <errno.h>
#include <pthread.h>
#include <sched.h>

#include <algorithm>

#include "base/check_op.h"
#include "base/logging.h"

namespace base
{

void InitThreading()
{
}

void TerminateOnThread()
{
}

size_t GetDefaultThreadStackSize(const pthread_attr_t &attributes)
{
    size_t size;
    CHECK_EQ(pthread_attr_getstacksize(&attributes, &size), 0);
    return std::max(size, size_t{2 * 1024 * 1024});
}

void PlatformThreadBase::SetName(const std::string &name)
{
    SetNameCommon(name);
    int error = pthread_setname_np(pthread_self(), name.substr(0, 15).c_str());
    if (error)
    {
        errno = error;
        PLOG(WARNING) << "pthread_setname_np";
    }
}

bool PlatformThreadBase::CanChangeThreadType(ThreadType from, ThreadType to)
{
    return from == to;
}

ThreadType PlatformThreadBase::GetCurrentEffectiveThreadTypeForTest()
{
    int policy;
    struct sched_param parameters;
    CHECK_EQ(pthread_getschedparam(pthread_self(), &policy, &parameters), 0);
    CHECK_EQ(policy, SCHED_OTHER);
    CHECK_EQ(parameters.sched_priority, 0);
    return ThreadType::kDefault;
}

namespace internal
{

void SetCurrentThreadTypeImpl(ThreadType thread_type, MessagePumpType)
{
    if (thread_type != ThreadType::kDefault)
    {
        DLOG(WARNING) << "Thread priority classes are not supported by the scheduler";
    }
}

PlatformPriorityOverride SetThreadTypeOverride(PlatformThreadHandle, ThreadType)
{
    return false;
}

void RemoveThreadTypeOverride(PlatformThreadHandle,
                              const PlatformPriorityOverride &priority_override_handle, ThreadType)
{
    CHECK(!priority_override_handle);
}

} // namespace internal
} // namespace base
