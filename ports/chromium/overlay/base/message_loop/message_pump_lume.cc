// SPDX-License-Identifier: BSD-3-Clause
#include "base/message_loop/message_pump_lume.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <unistd.h>

#include <algorithm>
#include <utility>
#include <vector>

#include "base/auto_reset.h"
#include "base/check_op.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/posix/eintr_wrapper.h"

namespace base
{

class MessagePumpLume::Registration : public RefCounted<Registration>
{
  public:
    Registration(uint64_t id, int fd, bool persistent, int mode, FdWatchController *controller,
                 FdWatcher *watcher)
        : id(id), fd(fd), persistent(persistent), mode(mode), controller(controller),
          watcher(watcher)
    {
    }

    const uint64_t id;
    const int fd;
    const bool persistent;
    const int mode;
    bool active = true;
    raw_ptr<FdWatchController> controller;
    raw_ptr<FdWatcher> watcher;

  private:
    friend class RefCounted<Registration>;
    ~Registration() = default;
};

MessagePumpLume::FdWatchController::FdWatchController(const Location &from_here)
    : FdWatchControllerInterface(from_here)
{
}

MessagePumpLume::FdWatchController::~FdWatchController()
{
    StopWatchingFileDescriptor();
}

bool MessagePumpLume::FdWatchController::StopWatchingFileDescriptor()
{
    auto registration = std::exchange(registration_, nullptr);
    if (registration)
    {
        registration->active = false;
        registration->controller = nullptr;
        registration->watcher = nullptr;
        if (pump_)
        {
            pump_->Remove(registration->id);
        }
    }
    pump_.reset();
    return true;
}

MessagePumpLume::MessagePumpLume()
{
    int descriptors[2];
    PCHECK(pipe2(descriptors, O_NONBLOCK | O_CLOEXEC) == 0);
    wake_read_.reset(descriptors[0]);
    wake_write_.reset(descriptors[1]);
}

MessagePumpLume::~MessagePumpLume()
{
    weak_factory_.InvalidateWeakPtrs();
    for (const auto &[id, registration] : registrations_)
    {
        registration->active = false;
        registration->controller = nullptr;
        registration->watcher = nullptr;
    }
}

bool MessagePumpLume::WatchFileDescriptor(int fd, bool persistent, int mode,
                                          FdWatchController *controller, FdWatcher *watcher)
{
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
    CHECK(controller);
    CHECK(watcher);
    if (controller->registration_ && controller->registration_->fd != fd)
    {
        controller->StopWatchingFileDescriptor();
        return false;
    }
    controller->StopWatchingFileDescriptor();
    if (fd < 0 || (mode & ~WATCH_READ_WRITE) || !mode || fcntl(fd, F_GETFD) < 0 ||
        next_id_ == UINT64_MAX)
    {
        return false;
    }
    auto registration =
        MakeRefCounted<Registration>(++next_id_, fd, persistent, mode, controller, watcher);
    registrations_.emplace(registration->id, registration);
    controller->registration_ = registration;
    controller->pump_ = weak_factory_.GetWeakPtr();
    return true;
}

void MessagePumpLume::Remove(uint64_t id)
{
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
    registrations_.erase(id);
}

bool MessagePumpLume::WaitForIO(TimeDelta delay)
{
    struct Snapshot
    {
        scoped_refptr<Registration> registration;
        size_t index;
    };

    std::map<int, size_t> indices;
    std::vector<struct pollfd> descriptors{{wake_read_.get(), POLLIN, 0}};
    std::vector<Snapshot> snapshots;
    for (const auto &[id, registration] : registrations_)
    {
        if (!registration->active)
        {
            continue;
        }
        auto [entry, inserted] = indices.emplace(registration->fd, descriptors.size());
        if (inserted)
        {
            descriptors.push_back({registration->fd, 0, 0});
        }
        short events = 0;
        if (registration->mode & WATCH_READ)
        {
            events |= POLLIN;
        }
        if (registration->mode & WATCH_WRITE)
        {
            events |= POLLOUT;
        }
        descriptors[entry->second].events |= events;
        snapshots.push_back({registration, entry->second});
    }
    int timeout =
        delay.is_max()
            ? -1
            : static_cast<int>(std::clamp<int64_t>(delay.InMillisecondsRoundedUp(), 0, INT_MAX));
    int result = poll(descriptors.data(), descriptors.size(), timeout);
    if (result < 0)
    {
        PCHECK(errno == EINTR);
        return false;
    }
    if (!result)
    {
        return false;
    }
    state_->delegate->BeginNativeWorkBeforeDoWork();
    if (descriptors[0].revents)
    {
        char buffer[256];
        ssize_t count = HANDLE_EINTR(read(wake_read_.get(), buffer, sizeof(buffer)));
        PCHECK(count > 0 || (count < 0 && errno == EAGAIN));
    }
    for (const auto &snapshot : snapshots)
    {
        const auto &registration = snapshot.registration;
        if (!registration->active || !registration->controller || !registration->watcher)
        {
            continue;
        }
        short events = descriptors[snapshot.index].revents;
        bool closed = events & (POLLERR | POLLHUP | POLLNVAL);
        bool readable = (registration->mode & WATCH_READ) && (closed || (events & POLLIN));
        bool writable = (registration->mode & WATCH_WRITE) && (closed || (events & POLLOUT));
        if (!readable && !writable)
        {
            continue;
        }
        if (!registration->persistent)
        {
            registration->active = false;
        }
        auto work_item = state_->delegate->BeginWorkItem();
        // The snapshot retains metadata when a callback removes or rearms any controller.
        if (writable && (!readable || registration->persistent))
        {
            registration->watcher->OnFileCanWriteWithoutBlocking(registration->fd);
            if (state_->quit)
            {
                break;
            }
        }
        if (readable && registration->controller && registration->watcher &&
            (registration->active || !registration->persistent))
        {
            registration->watcher->OnFileCanReadWithoutBlocking(registration->fd);
            if (state_->quit)
            {
                break;
            }
        }
    }
    return true;
}

void MessagePumpLume::Run(Delegate *delegate)
{
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
    RunState state{delegate};
    AutoReset<raw_ptr<RunState>> restore(&state_, &state);
    while (!state.quit)
    {
        auto next = delegate->DoWork();
        if (state.quit)
        {
            break;
        }
        bool native_work = WaitForIO(TimeDelta());
        if (state.quit)
        {
            break;
        }
        if (native_work || next.is_immediate())
        {
            continue;
        }
        delegate->DoIdleWork();
        if (state.quit)
        {
            break;
        }
        delegate->BeforeWait();
    if (state.quit) {
      break;
    }
        WaitForIO(next.remaining_delay());
    }
}

void MessagePumpLume::Quit()
{
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
    CHECK(state_);
    state_->quit = true;
}

void MessagePumpLume::ScheduleWork()
{
    char value = 1;
    ssize_t count = HANDLE_EINTR(write(wake_write_.get(), &value, sizeof(value)));
    PCHECK(count == sizeof(value) || (count < 0 && errno == EAGAIN));
}

void MessagePumpLume::ScheduleDelayedWork(const Delegate::NextWorkInfo &)
{
    ScheduleWork();
}

} // namespace base
