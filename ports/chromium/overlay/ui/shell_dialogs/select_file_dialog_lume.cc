// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/shell_dialogs/select_file_dialog.h"

#include <fcntl.h>
#include <lume/file_dialog.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdint>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/containers/span.h"
#include "base/files/scoped_file.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/posix/eintr_wrapper.h"
#include "base/process/launch.h"
#include "base/process/process.h"
#include "base/strings/string_util.h"
#include "base/strings/string_view_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/synchronization/lock.h"
#include "base/task/thread_pool.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/shell_dialogs/select_file_policy.h"
#include "ui/shell_dialogs/selected_file_info.h"

namespace ui
{
namespace
{

class DialogProcess : public base::RefCountedThreadSafe<DialogProcess>
{
  public:
    DialogProcess() = default;

    void Publish(base::ProcessHandle process)
    {
        base::AutoLock lock(lock_);
        process_ = process;
        if (cancelled_)
            kill(process_, SIGKILL);
    }

    void Cancel()
    {
        base::AutoLock lock(lock_);
        cancelled_ = true;
        if (process_)
            kill(process_, SIGKILL);
    }

    void BeforeReap()
    {
        base::AutoLock lock(lock_);
        // Clear the cancellation target before waitpid can release the PID.
        process_ = 0;
    }

  private:
    friend class base::RefCountedThreadSafe<DialogProcess>;
    ~DialogProcess() = default;
    base::Lock lock_;
    base::ProcessHandle process_ = 0;
    bool cancelled_ = false;
};

struct DialogResult
{
    bool valid = false;
    int filter = 0;
    std::vector<SelectedFileInfo> files;
};

void AppendU32(std::vector<uint8_t> &bytes, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i)
        bytes.push_back(static_cast<uint8_t>(value >> (i * 8)));
}

bool AppendString(std::vector<uint8_t> &bytes, const std::string &value, size_t limit)
{
    if (value.size() > limit || value.find('\0') != std::string::npos ||
        !base::IsStringUTF8(value) ||
        bytes.size() + 4 + value.size() > LUME_FILE_DIALOG_REQUEST_LIMIT)
        return false;
    AppendU32(bytes, static_cast<uint32_t>(value.size()));
    bytes.insert(bytes.end(), value.begin(), value.end());
    return true;
}

bool ConsumeU32(const std::vector<uint8_t> &bytes, size_t &position, uint32_t &value)
{
    if (bytes.size() - position < 4)
        return false;
    value = 0;
    for (unsigned i = 0; i < 4; ++i)
        value |= static_cast<uint32_t>(bytes[position++]) << (i * 8);
    return true;
}

DialogResult ParseResponse(const std::vector<uint8_t> &bytes, bool multiple,
                           uint32_t maximum_filter)
{
    DialogResult result;
    size_t position = 0;
    uint32_t magic, status, filter, count;
    if (!ConsumeU32(bytes, position, magic) || magic != LUME_FILE_DIALOG_RESPONSE_MAGIC ||
        !ConsumeU32(bytes, position, status) || status > 1 ||
        !ConsumeU32(bytes, position, filter) || filter > maximum_filter ||
        !ConsumeU32(bytes, position, count) || count > LUME_FILE_DIALOG_SELECTION_LIMIT ||
        (!multiple && count > 1) || (status == 0) != (count == 0))
        return result;
    for (uint32_t i = 0; i < count; ++i)
    {
        uint32_t length;
        if (!ConsumeU32(bytes, position, length) || !length ||
            length > LUME_FILE_DIALOG_PATH_LIMIT || length > bytes.size() - position)
            return DialogResult();
        std::string path(base::as_string_view(base::span(bytes).subspan(position, length)));
        position += length;
        if (!base::IsStringUTF8(path) || path.find('\0') != std::string::npos ||
            !base::FilePath(path).IsAbsolute())
            return DialogResult();
        result.files.emplace_back(base::FilePath(path));
    }
    if (position != bytes.size())
        return DialogResult();
    result.filter = static_cast<int>(filter);
    result.valid = true;
    return result;
}

bool ReapDialog(base::ProcessHandle process)
{
    base::TimeTicks deadline = base::TimeTicks::Now() + base::Seconds(5);
    for (;;)
    {
        int status = 0;
        pid_t result = HANDLE_EINTR(waitpid(process, &status, WNOHANG));
        if (result == process)
            return WIFEXITED(status) && WEXITSTATUS(status) == 0;
        if (result < 0)
            return false;
        if (base::TimeTicks::Now() >= deadline)
        {
            // This worker owns reaping and the zero result retains the child.
            kill(process, SIGKILL);
            HANDLE_EINTR(waitpid(process, &status, 0));
            return false;
        }
        base::PlatformThread::Sleep(base::Milliseconds(10));
    }
}

DialogResult RunDialog(std::vector<uint8_t> request, bool multiple, uint32_t maximum_filter,
                       scoped_refptr<DialogProcess> state)
{
    int descriptors[2];
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, descriptors) < 0)
    {
        PLOG(ERROR) << "Unable to create file dialog channel";
        return {};
    }
    base::ScopedFD parent(descriptors[0]);
    base::ScopedFD child(descriptors[1]);
    base::LaunchOptions options;
    options.fds_to_remap = {{child.get(), STDIN_FILENO}, {child.get(), STDOUT_FILENO}};
    base::Process process = base::LaunchProcess({LUME_FILE_DIALOG_PATH}, options);
    if (!process.IsValid())
    {
        LOG(ERROR) << "Unable to launch native file dialog";
        return {};
    }
    state->Publish(process.Handle());
    child.reset();
    bool success = true;
    size_t position = 0;
    while (position < request.size())
    {
        auto remaining = base::span(request).subspan(position);
        ssize_t count =
            HANDLE_EINTR(send(parent.get(), remaining.data(), remaining.size(), MSG_NOSIGNAL));
        if (count <= 0)
        {
            success = false;
            break;
        }
        position += static_cast<size_t>(count);
    }
    if (success && HANDLE_EINTR(shutdown(parent.get(), SHUT_WR)) < 0)
        success = false;
    std::vector<uint8_t> response;
    while (success)
    {
        uint8_t buffer[4096];
        ssize_t count = HANDLE_EINTR(recv(parent.get(), buffer, sizeof(buffer), 0));
        if (count < 0)
        {
            success = false;
            break;
        }
        if (!count)
            break;
        if (response.size() + static_cast<size_t>(count) > LUME_FILE_DIALOG_RESPONSE_LIMIT)
        {
            success = false;
            break;
        }
        auto received = base::span(buffer).first(static_cast<size_t>(count));
        response.insert(response.end(), received.begin(), received.end());
    }
    parent.reset();
    if (!success)
        kill(process.Handle(), SIGKILL);
    state->BeforeReap();
    bool exited = ReapDialog(process.Handle());
    if (!success || !exited)
    {
        LOG(ERROR) << "Native file dialog failed";
        return {};
    }
    return ParseResponse(response, multiple, maximum_filter);
}

class SelectFileDialogLume final : public SelectFileDialog
{
  public:
    SelectFileDialogLume(Listener *listener, std::unique_ptr<SelectFilePolicy> policy)
        : SelectFileDialog(listener, std::move(policy))
    {
    }

    bool IsRunning(gfx::NativeWindow owning_window) const override
    {
        return running_ && owner_ == owning_window;
    }

    void ListenerDestroyed() override
    {
        listener_ = nullptr;
        if (process_)
            process_->Cancel();
    }

  private:
    ~SelectFileDialogLume() override = default;

    bool HasMultipleFileTypeChoicesImpl() override
    {
        return multiple_filters_;
    }

    void SelectFileImpl(Type type, const std::u16string &title, const base::FilePath &default_path,
                        const FileTypeInfo *file_types, int file_type_index,
                        const base::FilePath::StringType &default_extension,
                        gfx::NativeWindow owning_window, const GURL *) override
    {
        CHECK(!running_);
        owner_ = owning_window;
        running_ = true;
        multiple_ = type == SELECT_OPEN_MULTI_FILE;
        std::vector<uint8_t> request;
        AppendU32(request, LUME_FILE_DIALOG_REQUEST_MAGIC);
        uint32_t action;
        switch (type)
        {
        case SELECT_FOLDER:
            action = LUME_FILE_DIALOG_FOLDER;
            break;
        case SELECT_UPLOAD_FOLDER:
            action = LUME_FILE_DIALOG_UPLOAD_FOLDER;
            break;
        case SELECT_EXISTING_FOLDER:
            action = LUME_FILE_DIALOG_EXISTING_FOLDER;
            break;
        case SELECT_SAVEAS_FILE:
            action = LUME_FILE_DIALOG_SAVE;
            break;
        case SELECT_OPEN_FILE:
            action = LUME_FILE_DIALOG_OPEN;
            break;
        case SELECT_OPEN_MULTI_FILE:
            action = LUME_FILE_DIALOG_MULTIPLE;
            break;
        default:
            Complete({});
            return;
        }
        size_t filters = file_types ? file_types->extensions.size() : 0;
        multiple_filters_ = filters + (file_types && file_types->include_all_files) > 1;
        if (filters > LUME_FILE_DIALOG_FILTER_LIMIT || file_type_index < 0 ||
            static_cast<size_t>(file_type_index) > filters + 1)
        {
            Complete({});
            return;
        }
        AppendU32(request, action);
        AppendU32(request, static_cast<uint32_t>(file_type_index));
        AppendU32(request, file_types && file_types->include_all_files ? 1 : 0);
        AppendU32(request, static_cast<uint32_t>(filters));
        uint64_t owner = 0;
        if (owning_window && owning_window->GetHost())
            owner = owning_window->GetHost()->GetAcceleratedWidget();
        AppendU32(request, static_cast<uint32_t>(owner));
        AppendU32(request, static_cast<uint32_t>(owner >> 32));
        bool valid = AppendString(request, base::UTF16ToUTF8(title), 4096) &&
                     AppendString(request, default_path.value(), LUME_FILE_DIALOG_PATH_LIMIT) &&
                     AppendString(request, default_extension, 255);
        for (size_t i = 0; valid && i < filters; ++i)
        {
            const auto &extensions = file_types->extensions[i];
            std::string description;
            if (i < file_types->extension_description_overrides.size())
                description = base::UTF16ToUTF8(file_types->extension_description_overrides[i]);
            if (description.empty() && !extensions.empty())
                description = "*." + extensions.front();
            valid = AppendString(request, description, 4096) && !extensions.empty() &&
                    extensions.size() <= LUME_FILE_DIALOG_PATTERN_LIMIT;
            if (!valid)
                break;
            AppendU32(request, static_cast<uint32_t>(extensions.size()));
            for (const auto &extension : extensions)
            {
                if (!AppendString(request, extension, 255))
                {
                    valid = false;
                    break;
                }
            }
        }
        if (!valid)
        {
            LOG(ERROR) << "Invalid native file dialog request";
            Complete({});
            return;
        }
        process_ = base::MakeRefCounted<DialogProcess>();
        base::ThreadPool::PostTaskAndReplyWithResult(
            FROM_HERE,
            {base::MayBlock(), base::TaskPriority::USER_BLOCKING,
             base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
            base::BindOnce(
                &RunDialog, std::move(request), type == SELECT_OPEN_MULTI_FILE,
                static_cast<uint32_t>(filters + (file_types && file_types->include_all_files)),
                process_),
            base::BindOnce(&SelectFileDialogLume::Complete, base::WrapRefCounted(this)));
    }

    void Complete(DialogResult result)
    {
        running_ = false;
        owner_ = nullptr;
        process_.reset();
        if (!listener_)
            return;
        if (!result.valid || result.files.empty())
        {
            listener_->FileSelectionCanceled();
        }
        else if (multiple_)
        {
            listener_->MultiFilesSelected(result.files);
        }
        else
        {
            listener_->FileSelected(result.files.front(), result.filter);
        }
    }

    gfx::NativeWindow owner_ = nullptr;
    scoped_refptr<DialogProcess> process_;
    bool running_ = false;
    bool multiple_filters_ = false;
    bool multiple_ = false;
};

} // namespace

SelectFileDialog *CreateSelectFileDialog(SelectFileDialog::Listener *listener,
                                         std::unique_ptr<SelectFilePolicy> policy)
{
    return new SelectFileDialogLume(listener, std::move(policy));
}

} // namespace ui
