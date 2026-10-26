// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "lld/Common/Driver.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/SmallVector.h"

LLD_HAS_DRIVER(elf)

extern "C" int app_main(unsigned argc, char **argv, char **)
{
    llvm::SmallVector<const char *, 32> arguments;
    arguments.append(argv, argv + argc);
    arguments.push_back("--no-mmap-output-file");
    arguments.push_back("--threads=1");
    auto result =
        lld::lldMain(arguments, llvm::outs(), llvm::errs(), {{lld::Gnu, &lld::elf::link}});
    return result.retCode;
}
