// SPDX-License-Identifier: BSD-3-Clause
#ifndef BASE_MESSAGE_LOOP_MESSAGE_PUMP_LUME_H_
#define BASE_MESSAGE_LOOP_MESSAGE_PUMP_LUME_H_

#include <cstdint>
#include <map>

#include "base/base_export.h"
#include "base/files/scoped_file.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_pump.h"
#include "base/message_loop/watchable_io_message_pump_posix.h"
#include "base/threading/thread_checker.h"

namespace base
{

class BASE_EXPORT MessagePumpLume : public MessagePump, public WatchableIOMessagePumpPosix
{
    class Registration;

  public:
    class FdWatchController : public FdWatchControllerInterface
    {
      public:
        explicit FdWatchController(const Location &from_here);
        ~FdWatchController() override;
        bool StopWatchingFileDescriptor() override;

      private:
        friend class MessagePumpLume;
        WeakPtr<MessagePumpLume> pump_;
        scoped_refptr<Registration> registration_;
    };

    MessagePumpLume();
    ~MessagePumpLume() override;
    bool WatchFileDescriptor(int fd, bool persistent, int mode, FdWatchController *controller,
                             FdWatcher *watcher);
    void Run(Delegate *delegate) override;
    void Quit() override;
    void ScheduleWork() override;
    void ScheduleDelayedWork(const Delegate::NextWorkInfo &next_work_info) override;

  private:
    struct RunState
    {
        raw_ptr<Delegate> delegate;
        bool quit = false;
    };

    bool WaitForIO(TimeDelta delay);
    void Remove(uint64_t id);
    std::map<uint64_t, scoped_refptr<Registration>> registrations_;
    uint64_t next_id_ = 0;
    ScopedFD wake_read_, wake_write_;
    raw_ptr<RunState> state_ = nullptr;
    THREAD_CHECKER(thread_checker_);
    WeakPtrFactory<MessagePumpLume> weak_factory_{this};
};

} // namespace base
#endif
