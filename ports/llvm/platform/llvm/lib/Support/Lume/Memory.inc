// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include <cstdlib>
#include "llvm/Support/Errc.h"

namespace llvm
{
namespace sys
{
MemoryBlock Memory::allocateMappedMemory(size_t N, const MemoryBlock *, unsigned F,
                                         std::error_code &E)
{
    if (F & MF_EXEC)
    {
        E = errc::function_not_supported;
        return {};
    }
    if (!N)
    {
        E.clear();
        return {};
    }
    void *P = ::malloc(N);
    if (!P)
    {
        E = errc::not_enough_memory;
        return {};
    }
    E.clear();
    return MemoryBlock(P, N);
}

std::error_code Memory::releaseMappedMemory(MemoryBlock &B)
{
    ::free(B.base());
    B = MemoryBlock();
    return {};
}

std::error_code Memory::protectMappedMemory(const MemoryBlock &, unsigned)
{
    return errc::function_not_supported;
}

void Memory::InvalidateInstructionCache(const void *, size_t)
{
}
}
}
