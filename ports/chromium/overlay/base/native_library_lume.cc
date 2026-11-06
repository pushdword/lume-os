// SPDX-License-Identifier: BSD-3-Clause
#include "base/native_library.h"

#include <errno.h>

#include "base/check.h"
#include "base/strings/strcat.h"

namespace base
{

std::string NativeLibraryLoadError::ToString() const
{
    return message;
}

NativeLibrary LoadNativeLibrary(const FilePath &, NativeLibraryLoadError *error)
{
    errno = ENOTSUP;
    if (error)
    {
        error->message = "Dynamic library loading is not supported";
    }
    return nullptr;
}

NativeLibrary LoadNativeLibraryWithOptions(const FilePath &path, const NativeLibraryOptions &,
                                           NativeLibraryLoadError *error)
{
    return LoadNativeLibrary(path, error);
}

void UnloadNativeLibrary(NativeLibrary library)
{
    CHECK(!library);
}

void *GetFunctionPointerFromNativeLibrary(NativeLibrary, const char *)
{
    errno = ENOTSUP;
    return nullptr;
}

std::string GetNativeLibraryName(std::string_view name)
{
    return StrCat({"lib", name, ".so"});
}

std::string GetLoadableModuleName(std::string_view name)
{
    return GetNativeLibraryName(name);
}

} // namespace base
