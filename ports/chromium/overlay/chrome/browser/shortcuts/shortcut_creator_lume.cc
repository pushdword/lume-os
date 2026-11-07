// Copyright 2012 The Chromium Authors
// Copyright 2018 The Chromium Authors
// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/shortcuts/shortcut_creator.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <string_view>
#include <utility>

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/containers/span.h"
#include "base/files/file.h"
#include "base/files/scoped_file.h"
#include "base/functional/callback.h"
#include "base/memory/ref_counted_memory.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "base/uuid.h"
#include "crypto/hash.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image.h"

namespace shortcuts
{
namespace
{

constexpr char kLauncher[] = "/usr/bin/browser";
constexpr char kChromium[] = "/data/libexec/chromium/chromium";
constexpr size_t kEntryLimit = 128 * 1024;
constexpr size_t kIconLimit = 16 * 1024 * 1024;

bool ValidValue(std::string_view value)
{
    for (const unsigned char character : value)
    {
        if (character < 0x20 || character == 0x7f)
        {
            return false;
        }
    }
    return true;
}

bool ValidPath(const base::FilePath &path)
{
    return path.IsAbsolute() && !path.ReferencesParent() && ValidValue(path.value());
}

base::ScopedFD OpenDirectory(const base::FilePath &path, bool create)
{
    if (!ValidPath(path))
    {
        return {};
    }
    constexpr int flags = O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC;
    base::ScopedFD directory(open("/", flags));
    if (!directory.is_valid())
    {
        return {};
    }
    for (const auto &component : path.GetComponents())
    {
        if (component == "/")
        {
            continue;
        }
        if (component.empty() || component == "." || component == "..")
        {
            return {};
        }
        int next = openat(directory.get(), component.c_str(), flags);
        if (next < 0 && create && errno == ENOENT)
        {
            if (mkdirat(directory.get(), component.c_str(), 0700) != 0 && errno != EEXIST)
            {
                return {};
            }
            next = openat(directory.get(), component.c_str(), flags);
        }
        if (next < 0)
        {
            return {};
        }
        directory.reset(next);
    }
    return directory;
}

bool SameFile(int directory, const std::string &name, const struct stat &created)
{
    struct stat current = {};
    return created.st_ino != 0 &&
           fstatat(directory, name.c_str(), &current, AT_SYMLINK_NOFOLLOW) == 0 &&
           S_ISREG(current.st_mode) && current.st_dev == created.st_dev &&
           current.st_ino == created.st_ino;
}

bool RemoveCreated(int directory, const std::string &name, const struct stat &created)
{
    return SameFile(directory, name, created) && unlinkat(directory, name.c_str(), 0) == 0;
}

bool WriteNewFile(int directory, const std::string &name, base::span<const uint8_t> contents,
                  struct stat *created)
{
    base::File file(openat(directory, name.c_str(),
                           O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600));
    if (!file.IsValid())
    {
        return false;
    }
    const bool identified =
        fstat(file.GetPlatformFile(), created) == 0 && S_ISREG(created->st_mode);
    const bool written = identified && file.WriteAtCurrentPosAndCheck(contents) && file.Flush();
    const bool closed = close(file.TakePlatformFile()) == 0;
    if (!written || !closed)
    {
        if (identified)
        {
            RemoveCreated(directory, name, *created);
        }
        return false;
    }
    return true;
}

std::string EscapeValue(std::string_view value)
{
    std::string result;
    for (const char character : value)
    {
        switch (character)
        {
        case '\\':
            result += "\\\\";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\t':
            result += "\\t";
            break;
        default:
            result += character;
            break;
        }
    }
    return result;
}

std::string QuoteExec(const base::CommandLine &command)
{
    std::string result;
    for (const auto &argument : command.argv())
    {
        if (!result.empty())
        {
            result += ' ';
        }
        result += '"';
        for (const char character : argument)
        {
            if (character == '%')
            {
                result += '%';
            }
            if (character == '"' || character == '`' || character == '$' || character == '\\')
            {
                result += '\\';
            }
            result += character;
        }
        result += '"';
    }
    return EscapeValue(result);
}

struct CreationResult
{
    base::FilePath path;
    ShortcutCreatorResult result = ShortcutCreatorResult::kError;
};

CreationResult CreateShortcut(ShortcutMetadata &metadata)
{
    const base::FilePath profile = metadata.profile_path.StripTrailingSeparators();
    const std::string title = base::UTF16ToUTF8(metadata.shortcut_title);
    if (!metadata.IsValid() || !ValidPath(profile) || profile.BaseName() == profile ||
        !ValidValue(title) || !ValidValue(metadata.shortcut_url.spec()) ||
        access(kLauncher, X_OK) != 0 || access(kChromium, X_OK) != 0)
    {
        return {};
    }

    base::FilePath desktop;
    if (!base::PathService::Get(base::DIR_USER_DESKTOP, &desktop) || !ValidPath(desktop))
    {
        return {};
    }
    base::ScopedFD profile_directory = OpenDirectory(profile, false);
    base::ScopedFD desktop_directory = OpenDirectory(desktop, true);
    if (!profile_directory.is_valid() || !desktop_directory.is_valid())
    {
        return {};
    }
    const std::string icon_directory_name(kWebShortcutsIconDirName);
    if (mkdirat(profile_directory.get(), icon_directory_name.c_str(), 0700) != 0 && errno != EEXIST)
    {
        return {};
    }
    base::ScopedFD icon_directory(openat(profile_directory.get(), icon_directory_name.c_str(),
                                         O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
    if (!icon_directory.is_valid())
    {
        return {};
    }

    const gfx::Image *image = metadata.shortcut_images.GetBest(gfx::Size(128, 128));
    if (!image)
    {
        return {};
    }
    scoped_refptr<base::RefCountedMemory> png = image->As1xPNGBytes();
    if (!png || png->size() == 0 || png->size() > kIconLimit)
    {
        return {};
    }

    const std::string identity = base::HexEncodeLower(
        crypto::hash::Sha256(profile.value() + "\n" + metadata.shortcut_url.spec()));
    const std::string stem =
        "lume-shortcut-" + identity + "-" + base::Uuid::GenerateRandomV4().AsLowercaseString();
    const std::string icon_name = stem + ".png";
    const std::string entry_name = stem + ".desktop";
    const base::FilePath icon_path =
        profile.Append(kWebShortcutsIconDirName).AppendASCII(icon_name);

    base::CommandLine command{base::FilePath(kLauncher)};
    command.AppendSwitchPath("user-data-dir", profile.DirName());
    command.AppendSwitchPath("profile-directory", profile.BaseName());
    command.AppendArg("--");
    command.AppendArg(metadata.shortcut_url.spec());
    const std::string contents =
        "[Desktop Entry]\nType=Application\nVersion=1.0\nTerminal=false\nName=" +
        EscapeValue(title) + "\nExec=" + QuoteExec(command) +
        "\nIcon=" + EscapeValue(icon_path.value()) +
        "\nX-Lume-Shortcut=true\nX-Lume-Shortcut-Identity=" + identity +
        "\nX-Lume-Shortcut-URL=" + EscapeValue(metadata.shortcut_url.spec()) + "\n";
    if (contents.size() > kEntryLimit)
    {
        return {};
    }

    struct stat created_icon = {};
    if (!WriteNewFile(icon_directory.get(), icon_name, static_cast<base::span<const uint8_t>>(*png),
                      &created_icon))
    {
        return {};
    }
    struct stat created_entry = {};
    if (!WriteNewFile(desktop_directory.get(), entry_name, base::as_byte_span(contents),
                      &created_entry))
    {
        RemoveCreated(icon_directory.get(), icon_name, created_icon);
        return {};
    }
    if (!SameFile(desktop_directory.get(), entry_name, created_entry) ||
        fchmodat(desktop_directory.get(), entry_name.c_str(), 0755, AT_SYMLINK_NOFOLLOW) != 0 ||
        !SameFile(desktop_directory.get(), entry_name, created_entry))
    {
        if (RemoveCreated(desktop_directory.get(), entry_name, created_entry))
        {
            RemoveCreated(icon_directory.get(), icon_name, created_icon);
        }
        return {};
    }
    return {desktop.AppendASCII(entry_name), ShortcutCreatorResult::kSuccess};
}

} // namespace

void CreateShortcutOnUserDesktop(ShortcutMetadata shortcut_metadata,
                                 ShortcutCreatorCallback complete)
{
    const CreationResult output = CreateShortcut(shortcut_metadata);
    std::move(complete).Run(output.path, output.result);
}

scoped_refptr<base::SequencedTaskRunner> GetShortcutsTaskRunner()
{
    return base::ThreadPool::CreateSequencedTaskRunner(
        {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
         base::TaskShutdownBehavior::BLOCK_SHUTDOWN});
}

} // namespace shortcuts
