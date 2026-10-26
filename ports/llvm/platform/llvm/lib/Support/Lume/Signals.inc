// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Lume currently delivers process faults as termination, not Unix signals.
// Stack symbolization is unavailable.
#include "llvm/Support/Process.h"

static bool findModulesAndOffsets(void **, int, const char **, intptr_t *, const char *,
                                  StringSaver &)
{
    return false;
}

static bool printMarkupContext(raw_ostream &, const char *)
{
    return false;
}

namespace llvm
{
namespace sys
{
bool RemoveFileOnSignal(StringRef, std::string *)
{
    return false;
}

void DontRemoveFileOnSignal(StringRef)
{
}

void PrintStackTraceOnErrorSignal(StringRef, bool)
{
}

void DisableSystemDialogsOnCrash()
{
}

void PrintStackTrace(raw_ostream &OS, int)
{
    OS << "Lume: native stack trace unavailable\n";
}

void AddSignalHandler(SignalHandlerCallback F, void *C, bool)
{
    insertSignalHandler(F, C);
}

void SetInterruptFunction(void (*)())
{
}

void SetInfoSignalFunction(void (*)())
{
}

void SetOneShotPipeSignalFunction(void (*)())
{
}

void DefaultOneShotPipeSignalHandler()
{
    Process::Exit(74);
}

void CleanupOnSignal(uintptr_t)
{
    RunSignalHandlers();
}

void unregisterHandlers()
{
}

void RunInterruptHandlers()
{
}
}
}
