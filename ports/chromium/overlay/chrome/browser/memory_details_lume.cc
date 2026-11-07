// SPDX-License-Identifier: BSD-3-Clause
#include "chrome/browser/memory_details.h"

#include <algorithm>
#include <utility>

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/process/process_handle.h"
#include "base/process/process_metrics.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/single_thread_task_runner.h"
#include "base/version_info/version_info.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/process_type.h"

MemoryDetails::MemoryDetails() : process_data_(1)
{
}

ProcessData *MemoryDetails::ChromeBrowser()
{
    return &process_data_[0];
}

void MemoryDetails::CollectProcessData(const std::vector<ProcessMemoryInformation> &child_info)
{
    const base::ProcessId current = base::GetCurrentProcId();
    ProcessData browser;
    browser.name = base::ASCIIToUTF16(version_info::GetProductName());
    browser.process_name = u"chrome";
    browser.processes = child_info;
    std::erase_if(browser.processes,
                  [current](const ProcessMemoryInformation &process)
                  {
                      return !process.pid || process.pid == current;
                  });
    for (auto &process : browser.processes)
    {
        process.num_processes = 1;
    }

    ProcessMemoryInformation own;
    own.pid = current;
    own.num_processes = 1;
    own.process_type = content::PROCESS_TYPE_BROWSER;
    auto metrics = base::ProcessMetrics::CreateCurrentProcessMetrics();
    own.num_open_fds = metrics->GetOpenFdCount();
    own.open_fds_soft_limit = metrics->GetOpenFdSoftLimit();
    browser.processes.push_back(std::move(own));
    process_data_.assign(1, browser);

    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE, base::BindOnce(&MemoryDetails::CollectChildInfoOnUIThread, this));
}
