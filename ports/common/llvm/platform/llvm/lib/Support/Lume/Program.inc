// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "llvm/Support/Errc.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/ADT/SmallString.h"
#include <cstdint>
extern "C" int32_t lume_syscall(uint32_t, uint32_t, uint32_t, uint32_t);

ProcessInfo::ProcessInfo() : Pid(0), ReturnCode(0)
{
}

ErrorOr<std::string> sys::findProgramByName(StringRef N, ArrayRef<StringRef> Paths)
{
    if (N.contains('/'))
        return N.str();
    for (StringRef P : Paths)
    {
        SmallString<128> V(P);
        sys::path::append(V, N);
        if (sys::fs::can_execute(V))
            return V.str().str();
    }
    return errc::no_such_file_or_directory;
}

static bool Execute(ProcessInfo &PI, StringRef P, ArrayRef<StringRef> A,
                    std::optional<ArrayRef<StringRef>> Env, ArrayRef<std::optional<StringRef>> R,
                    unsigned Limit, std::string *E, BitVector *Affinity, bool Detach)
{
    if (!R.empty() || Limit || Affinity || Detach || A.size() > 8 || (Env && Env->size() > 16))
    {
        if (E)
            *E = "Lume spawn options unsupported";
        return false;
    }
    SmallVector<std::string, 8> Store;
    for (auto S : A)
        Store.push_back(S.str());
    const char *Argv[8];
    for (unsigned I = 0; I < A.size(); ++I)
        Argv[I] = Store[I].c_str();
    std::string Path = P.str();
    uint32_t Request[5] = {(uint32_t)(uintptr_t)Path.c_str(), (uint32_t)A.size(),
                           (uint32_t)(uintptr_t)Argv, 0, 0};
    SmallVector<std::string, 16> EnvStore;
    const char *Envp[16];
    if (Env)
    {
        for (auto S : *Env)
            EnvStore.push_back(S.str());
        for (unsigned I = 0; I < EnvStore.size(); ++I)
            Envp[I] = EnvStore[I].c_str();
        Request[3] = EnvStore.size();
        Request[4] = (uint32_t)(uintptr_t)Envp;
    }
    int Id = lume_syscall(27, (uint32_t)(uintptr_t)Request, 0, 0);
    if (Id < 0)
    {
        if (E)
            *E = "Lume spawn failed: " + std::to_string(Id);
        return false;
    }
    PI.Pid = Id;
    return true;
}

bool sys::commandLineFitsWithinSystemLimits(StringRef, ArrayRef<StringRef> A)
{
    size_t N = 0;
    for (auto S : A)
        N += S.size() + 1;
    return A.size() <= 8 && N <= 512;
}

std::error_code sys::ChangeStdinMode(fs::OpenFlags)
{
    return {};
}

std::error_code sys::ChangeStdoutMode(fs::OpenFlags)
{
    return {};
}

std::error_code sys::ChangeStdinToBinary()
{
    return {};
}

std::error_code sys::ChangeStdoutToBinary()
{
    return {};
}

std::error_code sys::writeFileWithEncoding(StringRef F, StringRef C, WindowsEncodingMethod)
{
    std::error_code E;
    raw_fd_ostream O(F, E);
    if (E)
        return E;
    O << C;
    O.close();
    return O.error();
}

ProcessInfo sys::Wait(const ProcessInfo &PI, std::optional<unsigned> Timeout, std::string *E,
                      std::optional<ProcessStatistics> *Stats, bool Polling)
{
    ProcessInfo R;
    uint32_t Status[2];
    if (Timeout && (*Timeout || !Polling))
    {
        if (E)
            *E = "Lume timed wait unsupported";
        return R;
    }
    int N = lume_syscall(26, PI.Pid, (uint32_t)(uintptr_t)Status, 0);
    if (N < 0)
    {
        if (E)
            *E = "Lume wait failed";
        return R;
    }
    R.Pid = Status[0];
    R.ReturnCode = Status[1];
    if (Stats)
        Stats->reset();
    return R;
}
