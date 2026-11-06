// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/child_process_launcher_helper.h"

#include <utility>

#include "base/base_paths.h"
#include "base/check.h"
#include "base/command_line.h"
#include "base/path_service.h"
#include "base/posix/global_descriptors.h"
#include "content/browser/child_process_launcher.h"
#include "content/browser/child_process_launcher_helper_posix.h"
#include "content/browser/sandboxed_process_launcher_delegate.h"
#include "content/public/browser/child_process_launcher_utils.h"
#include "content/public/common/result_codes.h"

namespace content
{
namespace internal
{

std::optional<mojo::NamedPlatformChannel>
ChildProcessLauncherHelper::CreateNamedPlatformChannelOnLauncherThread()
{
    CHECK(CurrentlyOnProcessLauncherTaskRunner(), base::NotFatalUntil::M159);
    return std::nullopt;
}

void ChildProcessLauncherHelper::BeforeLaunchOnClientThread()
{
    CHECK(client_task_runner_->RunsTasksInCurrentSequence(), base::NotFatalUntil::M159);
}

std::unique_ptr<FileMappedForLaunch> ChildProcessLauncherHelper::GetFilesToMap()
{
    CHECK(CurrentlyOnProcessLauncherTaskRunner(), base::NotFatalUntil::M159);
    return CreateDefaultPosixFilesToMap(child_process_id(), mojo_channel_->remote_endpoint(),
                                        file_data_->files_to_preload, GetProcessType(),
                                        command_line());
}

bool ChildProcessLauncherHelper::IsUsingLaunchOptions()
{
    return true;
}

bool ChildProcessLauncherHelper::BeforeLaunchOnLauncherThread(
    FileMappedForLaunch &files_to_register, base::LaunchOptions *options)
{
    CHECK(CurrentlyOnProcessLauncherTaskRunner(), base::NotFatalUntil::M159);
    CHECK(options);
    options->fds_to_remap =
        files_to_register.GetMappingWithIDAdjustment(base::GlobalDescriptors::kBaseDescriptor);
    options->environment = delegate_->GetEnvironment();
    return true;
}

ChildProcessLauncherHelper::Process ChildProcessLauncherHelper::LaunchProcessOnLauncherThread(
    const base::LaunchOptions *options, std::unique_ptr<FileMappedForLaunch> files_to_register,
    bool *is_synchronous_launch, int *launch_result)
{
    CHECK(CurrentlyOnProcessLauncherTaskRunner(), base::NotFatalUntil::M159);
    CHECK(options);
    CHECK(files_to_register);
    *is_synchronous_launch = true;
    Process process;
    process.process = base::LaunchProcess(*command_line(), *options);
    *launch_result = process.process.IsValid() ? LAUNCH_RESULT_SUCCESS : LAUNCH_RESULT_FAILURE;
    return process;
}

void ChildProcessLauncherHelper::AfterLaunchOnLauncherThread(
    const ChildProcessLauncherHelper::Process &, const base::LaunchOptions *)
{
    file_data_.reset();
}

ChildProcessTerminationInfo
ChildProcessLauncherHelper::GetTerminationInfo(const ChildProcessLauncherHelper::Process &process,
                                               bool known_dead)
{
    ChildProcessTerminationInfo info;
    info.status =
        known_dead ? base::GetKnownDeadTerminationStatus(process.process.Handle(), &info.exit_code)
                   : base::GetTerminationStatus(process.process.Handle(), &info.exit_code);
    return info;
}

// static
bool ChildProcessLauncherHelper::TerminateProcess(const base::Process &process, int exit_code)
{
    return process.Terminate(exit_code, false);
}

// static
void ChildProcessLauncherHelper::ForceNormalProcessTerminationSync(
    ChildProcessLauncherHelper::Process process)
{
    CHECK(CurrentlyOnProcessLauncherTaskRunner(), base::NotFatalUntil::M159);
    process.process.Terminate(RESULT_CODE_NORMAL_EXIT, false);
    base::EnsureProcessTerminated(std::move(process.process));
}

// static
void ChildProcessLauncherHelper::ApplyProcessPriorityOnLauncherThread(
    base::Process &process, base::Process::Priority priority)
{
    CHECK(CurrentlyOnProcessLauncherTaskRunner(), base::NotFatalUntil::M159);
    if (process.CanSetPriority())
        process.SetPriority(priority);
}

base::File OpenFileToShare(const base::FilePath &path, base::MemoryMappedFile::Region *region)
{
    base::FilePath exe_dir;
    CHECK(base::PathService::Get(base::DIR_EXE, &exe_dir), base::NotFatalUntil::M159);
    *region = base::MemoryMappedFile::Region::kWholeFile;
    return base::File(exe_dir.Append(path), base::File::FLAG_OPEN | base::File::FLAG_READ);
}

} // namespace internal
} // namespace content
