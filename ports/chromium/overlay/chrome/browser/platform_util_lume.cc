// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/platform_util.h"

#include <errno.h>
#include <lume/file_dialog.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "base/process/launch.h"
#include "base/process/process.h"
#include "base/strings/string_util.h"
#include "base/task/thread_pool.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"
#include "chrome/browser/platform_util_internal.h"
#include "content/public/browser/browser_thread.h"
#include "url/gurl.h"

namespace platform_util
{
namespace
{

constexpr base::TaskTraits kShellTaskTraits = {base::MayBlock(), base::TaskPriority::USER_BLOCKING,
                                               base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN};

bool RunOpenHelper(const std::string &operation, const std::string &argument)
{
    base::Process process =
        base::LaunchProcess({LUME_FILE_DIALOG_PATH, operation, argument}, base::LaunchOptions());
    if (!process.IsValid())
    {
        LOG(ERROR) << "Unable to launch desktop item handler";
        return false;
    }
    base::TimeTicks deadline = base::TimeTicks::Now() + base::Seconds(30);
    for (;;)
    {
        int status = 0;
        pid_t result = HANDLE_EINTR(waitpid(process.Handle(), &status, WNOHANG));
        if (result == process.Handle())
            return WIFEXITED(status) && WEXITSTATUS(status) == 0;
        if (result < 0)
            return false;
        if (base::TimeTicks::Now() >= deadline)
        {
            kill(process.Handle(), SIGKILL);
            HANDLE_EINTR(waitpid(process.Handle(), &status, 0));
            LOG(ERROR) << "Desktop item handler timed out";
            return false;
        }
        base::PlatformThread::Sleep(base::Milliseconds(10));
    }
}

bool LaunchFileManager(const base::FilePath &path)
{
    base::Process process =
        base::LaunchProcess({"/data/bin/Thunar", "--", path.value()}, base::LaunchOptions());
    if (!process.IsValid())
    {
        LOG(ERROR) << "Unable to launch file manager";
        return false;
    }
    base::ThreadPool::PostTask(FROM_HERE,
                               {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
                                base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
                               base::BindOnce(
                                   [](base::Process child)
                                   {
                                       HANDLE_EINTR(waitpid(child.Handle(), nullptr, 0));
                                   },
                                   std::move(process)));
    return true;
}

OpenOperationResult OpenVerifiedItem(const base::FilePath &path, OpenItemType type)
{
    internal::RunOpenItemThreadObserverForTesting();
    if (!path.IsAbsolute() || path.value().find('\0') != std::string::npos ||
        !base::IsStringUTF8(path.value()))
        return OPEN_FAILED_FILE_ERROR;
    struct stat information;
    if (stat(path.value().c_str(), &information) < 0)
        return errno == ENOENT || errno == ENOTDIR ? OPEN_FAILED_PATH_NOT_FOUND
                                                   : OPEN_FAILED_FILE_ERROR;
    if ((type == OPEN_FOLDER && !S_ISDIR(information.st_mode)) ||
        (type == OPEN_FILE && !S_ISREG(information.st_mode)))
        return OPEN_FAILED_INVALID_TYPE;
    if (!internal::AreShellOperationsAllowed())
        return OPEN_FAILED_FILE_ERROR;
    bool launched =
        type == OPEN_FOLDER ? LaunchFileManager(path) : RunOpenHelper("--open-file", path.value());
    return launched ? OPEN_SUCCEEDED : OPEN_FAILED_FILE_ERROR;
}

} // namespace

void OpenItem(Profile *, const base::FilePath &full_path, OpenItemType item_type,
              OpenOperationCallback callback)
{
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    base::ThreadPool::PostTaskAndReplyWithResult(
        FROM_HERE, kShellTaskTraits, base::BindOnce(&OpenVerifiedItem, full_path, item_type),
        base::BindOnce(
            [](OpenOperationCallback completion, OpenOperationResult result)
            {
                if (completion)
                    std::move(completion).Run(result);
            },
            std::move(callback)));
}

void ShowItemInFolder(Profile *, const base::FilePath &full_path)
{
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    if (!internal::AreShellOperationsAllowed())
        return;
    base::ThreadPool::PostTask(FROM_HERE, kShellTaskTraits,
                               base::BindOnce(
                                   [](base::FilePath path)
                                   {
                                       if (path.IsAbsolute() &&
                                           path.value().find('\0') == std::string::npos &&
                                           base::IsStringUTF8(path.value()))
                                           LaunchFileManager(path);
                                   },
                                   full_path));
}

void OpenExternal(const GURL &url)
{
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    if (!internal::AreShellOperationsAllowed() || !url.is_valid())
        return;
    base::ThreadPool::PostTask(FROM_HERE, kShellTaskTraits,
                               base::BindOnce(
                                   [](std::string uri)
                                   {
                                       if (!RunOpenHelper("--open-uri", uri))
                                           LOG(ERROR) << "Unable to open external URI";
                                   },
                                   url.spec()));
}

} // namespace platform_util
