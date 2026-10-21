// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "llvm/Support/LLVMDriver.h"

int LUME_TOOL_MAIN(int, char **, const llvm::ToolContext &);

extern "C" int app_main(unsigned argc, char **argv, char **)
{
    const llvm::ToolContext context = {argv[0], nullptr, false};
    return LUME_TOOL_MAIN((int)argc, argv, context);
}
