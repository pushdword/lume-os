// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "llvm/Support/Errno.h"
#include <cstdlib>
#include <unistd.h>
extern "C" int32_t lume_syscall(uint32_t, uint32_t, uint32_t, uint32_t);

namespace llvm
{
namespace sys
{
Process::Pid Process::getProcessId()
{
    return lume_syscall(7, 0, 0, 0);
}

Expected<unsigned> Process::getPageSize()
{
    return 4096u;
}

size_t Process::GetMallocUsage()
{
    return 0;
}

void Process::GetTimeUsage(TimePoint<> &E, std::chrono::nanoseconds &U, std::chrono::nanoseconds &S)
{
    E = TimePoint<>(std::chrono::milliseconds((uint32_t)lume_syscall(9, 0, 0, 0)));
    U = {};
    S = {};
}

void Process::PreventCoreFiles()
{
    coreFilesPrevented = true;
}

std::optional<std::string> Process::GetEnv(StringRef N)
{
    const char *V = ::getenv(N.str().c_str());
    return V ? std::optional<std::string>(V) : std::nullopt;
}

std::error_code Process::FixupStandardFileDescriptors()
{
    return {};
}

std::error_code Process::SafelyCloseFileDescriptor(int FD)
{
    return ::close(FD) < 0 ? errnoAsErrorCode() : std::error_code();
}

bool Process::StandardInIsUserInput()
{
    return true;
}

bool Process::StandardOutIsDisplayed()
{
    return true;
}

bool Process::StandardErrIsDisplayed()
{
    return true;
}

bool Process::FileDescriptorIsDisplayed(int FD)
{
    return FD >= 0 && FD <= 2;
}

bool Process::FileDescriptorHasColors(int)
{
    return false;
}

unsigned Process::StandardOutColumns()
{
    return 128;
}

unsigned Process::StandardErrColumns()
{
    return 128;
}

bool Process::StandardOutHasColors()
{
    return false;
}

bool Process::StandardErrHasColors()
{
    return false;
}

void Process::UseANSIEscapeCodes(bool)
{
}

bool Process::ColorNeedsFlush()
{
    return false;
}

const char *Process::OutputColor(char, bool, bool)
{
    return "";
}

const char *Process::OutputBold(bool)
{
    return "";
}

const char *Process::OutputReverse()
{
    return "";
}

const char *Process::ResetColor()
{
    return "";
}

unsigned Process::GetRandomNumber()
{
    static uint32_t X
    = 0;
    if (!X)
        X
    = (uint32_t)lume_syscall(9, 0, 0, 0) ^ ((uint32_t)getProcessId() << 16) ^ 0x9e3779b9u;
    X
    ^= X
    << 13;
    X
    ^= X
    >> 17;
    X
    ^= X
    << 5;
    return X;
}

[[noreturn]] void Process::ExitNoCleanup(int R)
{
    ::_Exit(R);
}
}
}
