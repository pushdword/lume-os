// SPDX-License-Identifier: BSD-3-Clause
#include "chrome/common/chrome_paths_internal.h"

#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <optional>
#include <string>

#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/no_destructor.h"
#include "base/strings/string_util.h"

namespace chrome
{
namespace
{

std::optional<base::FilePath> FindPersistentHome()
{
    auto environment = base::Environment::Create();
    const auto configured = environment->GetVar("HOME");
    const std::string input = configured && !configured->empty() ? *configured : "/data";
    const base::FilePath requested(input);
    if (!base::IsStringUTF8(input) || !requested.IsAbsolute() || requested.ReferencesParent())
    {
        return std::nullopt;
    }
    char resolved[PATH_MAX];
    if (!realpath(input.c_str(), resolved))
    {
        return std::nullopt;
    }
    const base::FilePath home(resolved), data("/data");
    struct stat information;
    if ((home != data && !data.IsParent(home)) || stat(resolved, &information) ||
        !S_ISDIR(information.st_mode) || information.st_uid != geteuid() ||
        (information.st_mode & 0022))
    {
        return std::nullopt;
    }
    return home;
}

const std::optional<base::FilePath> &PersistentHome()
{
    static const base::NoDestructor<std::optional<base::FilePath>> home(FindPersistentHome());
    return *home;
}

} // namespace

bool GetDefaultUserDataDirectory(base::FilePath *result)
{
    const auto &home = PersistentHome();
    if (!home)
    {
        return false;
    }
    *result = home->Append(".config").Append("chromium");
    return true;
}

void GetUserCacheDirectory(const base::FilePath &profile_dir, base::FilePath *result)
{
    *result = profile_dir;
}

bool GetUserDocumentsDirectory(base::FilePath *)
{
    return false;
}

bool GetUserDownloadsDirectory(base::FilePath *result)
{
    *result = base::FilePath("/data/downloads");
    return true;
}

bool GetUserMusicDirectory(base::FilePath *)
{
    return false;
}

bool GetUserPicturesDirectory(base::FilePath *)
{
    return false;
}

bool GetUserVideosDirectory(base::FilePath *)
{
    return false;
}

bool ProcessNeedsProfileDir(const std::string &process_type)
{
    return process_type.empty();
}

} // namespace chrome
