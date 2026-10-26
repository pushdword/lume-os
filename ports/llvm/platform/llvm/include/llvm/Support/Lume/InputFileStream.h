// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef LLVM_SUPPORT_LUME_INPUTFILESTREAM_H
#define LLVM_SUPPORT_LUME_INPUTFILESTREAM_H
#include "llvm/Support/MemoryBuffer.h"
#include <sstream>

namespace llvm::lume
{
/// Lume's libc++ profile has string streams, but no std::filesystem/fstream.
/// Preserve the upstream line parser through a real buffered file read.
class InputFileStream : public std::istringstream
{
    bool Open = false;

  public:
    explicit InputFileStream(StringRef Path)
    {
        auto Buffer = MemoryBuffer::getFile(Path);
        if (!Buffer)
        {
            setstate(std::ios_base::failbit);
            return;
        }
        str((*Buffer)->getBuffer().str());
        Open = true;
    }

    bool is_open() const
    {
        return Open;
    }
};
} // namespace llvm::lume
#endif
