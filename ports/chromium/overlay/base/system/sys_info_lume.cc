// SPDX-License-Identifier: BSD-3-Clause
#include "base/system/sys_info.h"

#include <unistd.h>

#include "base/byte_size.h"
#include "base/check_op.h"

namespace base
{

namespace
{
ByteSize MemoryBytes(int query)
{
    long pages = sysconf(query);
    long page_size = sysconf(_SC_PAGESIZE);
    CHECK_GE(pages, 0);
    CHECK_GT(page_size, 0);
    return ByteSize(static_cast<uint64_t>(pages)) * page_size;
}
} // namespace

ByteSize SysInfo::AmountOfTotalPhysicalMemoryImpl()
{
    return MemoryBytes(_SC_PHYS_PAGES);
}

ByteSize SysInfo::AmountOfAvailablePhysicalMemoryImpl()
{
    return MemoryBytes(_SC_AVPHYS_PAGES);
}

std::string SysInfo::CPUModelName()
{
    return std::string();
}

SysInfo::HardwareInfo SysInfo::GetHardwareInfoSync()
{
    return HardwareInfo();
}

} // namespace base
