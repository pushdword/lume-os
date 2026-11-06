// SPDX-License-Identifier: BSD-3-Clause
#include "base/profiler/module_cache.h"

#include "base/process/process_handle.h"

extern "C"
{
    extern const char __lume_text_start[];
    extern const char __lume_text_end[];
}

namespace base
{
namespace
{

class LumeModule final : public ModuleCache::Module
{
  public:
    LumeModule(uintptr_t start, size_t size, FilePath name)
        : start_(start), size_(size), name_(std::move(name))
    {
    }

    uintptr_t GetBaseAddress() const override
    {
        return start_;
    }

    std::string GetId() const override
    {
        return {};
    }

    FilePath GetDebugBasename() const override
    {
        return name_;
    }

    size_t GetSize() const override
    {
        return size_;
    }

    bool IsNative() const override
    {
        return true;
    }

  private:
    const uintptr_t start_;
    const size_t size_;
    const FilePath name_;
};

} // namespace

std::unique_ptr<const ModuleCache::Module> ModuleCache::CreateModuleForAddress(uintptr_t address)
{
    uintptr_t start = reinterpret_cast<uintptr_t>(__lume_text_start);
    uintptr_t end = reinterpret_cast<uintptr_t>(__lume_text_end);
    if (address < start || address >= end)
    {
        return nullptr;
    }
    FilePath executable = GetProcessExecutablePath(GetCurrentProcessHandle());
    if (executable.empty())
    {
        return nullptr;
    }
    return std::make_unique<LumeModule>(start, end - start, executable.BaseName());
}

} // namespace base
