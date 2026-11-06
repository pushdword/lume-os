// Copyright 2012 The Chromium Authors
// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/web_applications/os_integration/web_app_shortcut.h"

#include <unistd.h>

#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/functional/bind.h"
#include "base/hash/sha1.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ref_counted_memory.h"
#include "base/nix/xdg_util.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/task_runner.h"
#include "base/threading/scoped_blocking_call.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/browser/web_applications/os_integration/web_app_file_handler_registration.h"
#include "chrome/browser/web_applications/os_integration/web_app_run_on_os_login.h"
#include "chrome/browser/web_applications/web_app_constants.h"

namespace web_app
{
namespace
{

constexpr char kLauncher[] = "/usr/bin/browser";
constexpr char kChromium[] = "/data/libexec/chromium/chromium";
constexpr size_t kEntryLimit = 128 * 1024;
constexpr char kMenuDigest[] = "FAB538B5D1BB05B4F0C6BD343DE588E33F334C44";
constexpr char kMenuDirectory[] = "lume-chromium-apps.directory";
constexpr char kDirectoryContents[] =
    "[Desktop Entry]\nType=Directory\nName=Chromium Apps\nX-Lume-WebApps-Directory=true\n";

std::string IdentityHash(std::string_view value)
{
    const std::string digest = base::SHA1HashString(value);
    return base::HexEncode(digest);
}

struct Identity
{
    std::string profile;
    std::string app;

    std::string Prefix() const
    {
        return "lume-webapp-" + profile + "-";
    }

    std::string Stem() const
    {
        return Prefix() + app;
    }

    std::string Marker() const
    {
        return "\nX-Lume-WebApp-Profile=" + profile + "\nX-Lume-WebApp-Id=" + app + "\n";
    }
};

std::optional<Identity> MakeIdentity(const base::FilePath &profile, std::string_view app)
{
    if (!profile.IsAbsolute() || profile.ReferencesParent() ||
        profile.value().find('\0') != std::string::npos || app.empty())
        return std::nullopt;
    return Identity{IdentityHash(profile.StripTrailingSeparators().value()), IdentityHash(app)};
}

std::optional<Identity> MakeIdentity(const ShortcutInfo &info)
{
    return MakeIdentity(info.profile_path, GenerateApplicationNameFromInfo(info));
}

struct Locations
{
    base::FilePath desktop;
    base::FilePath applications;
    base::FilePath startup;
    base::FilePath icons;
    base::FilePath config;
    base::FilePath fragments;
    base::FilePath directories;
};

bool ValidDirectory(const base::FilePath &path)
{
    return path.IsAbsolute() && !path.ReferencesParent() &&
           path.value().find('\0') == std::string::npos;
}

Locations GetLocations()
{
    auto env = base::Environment::Create();
    Locations locations;
    base::PathService::Get(base::DIR_USER_DESKTOP, &locations.desktop);
    const base::FilePath data = base::nix::GetXDGDataWriteLocation(env.get());
    const base::FilePath config =
        base::nix::GetXDGDirectory(env.get(), "XDG_CONFIG_HOME", ".config");
    if (ValidDirectory(data))
    {
        locations.applications = data.Append("applications");
        locations.icons = data.Append("lume-chromium").Append("web-app-icons");
        locations.directories = data.Append("desktop-directories");
    }
    if (ValidDirectory(config))
    {
        locations.config = config;
        locations.startup = config.Append("autostart");
        locations.fragments = config.Append("menus").Append("applications-merged");
    }
    if (!ValidDirectory(locations.desktop))
        locations.desktop.clear();
    return locations;
}

bool MenuSupported(const Locations &paths)
{
    if (paths.config.empty() || paths.directories.empty())
        return false;
    auto env = base::Environment::Create();
    const std::string prefix = env->GetVar("XDG_MENU_PREFIX").value_or("xfce-");
    if (prefix.find_first_not_of(
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") !=
        std::string::npos)
        return false;
    const std::string filename = prefix + "applications.menu";
    std::vector<base::FilePath> directories{paths.config};
    const auto configured_dirs = env->GetVar("XDG_CONFIG_DIRS");
    const std::string config_dirs =
        !configured_dirs || configured_dirs->empty() ? "/etc/xdg" : *configured_dirs;
    size_t begin = 0;
    while (begin <= config_dirs.size())
    {
        const size_t end = config_dirs.find(':', begin);
        const base::FilePath directory(config_dirs.substr(begin, end - begin));
        if (ValidDirectory(directory))
            directories.push_back(directory);
        if (end == std::string::npos)
            break;
        begin = end + 1;
    }
    for (const auto &directory : directories)
    {
        const base::FilePath menu = directory.Append("menus").AppendASCII(filename);
        if (!base::PathExists(menu))
            continue;
        std::string contents;
        // Only the default menu contract guarantees that these fragments are consumed.
        return base::ReadFileToStringWithMaxSize(menu, &contents, kEntryLimit) &&
               IdentityHash(contents) == kMenuDigest;
    }
    return false;
}

base::FilePath FragmentPath(const Locations &paths, const Identity &identity)
{
    return paths.fragments.empty() ? base::FilePath()
                                   : paths.fragments.AppendASCII(identity.Stem() + ".menu");
}

std::string FragmentContents(const Identity &identity)
{
    const std::string filename = identity.Stem() + ".desktop";
    return "<!-- Lume-WebApp " + identity.profile + " " + identity.app +
           " -->\n"
           "<!DOCTYPE Menu PUBLIC \"-//freedesktop//DTD Menu 1.0//EN\" "
           "\"http://www.freedesktop.org/standards/menu-spec/1.0/menu.dtd\">\n"
           "<Menu><Name>Applications</Name><Exclude><Filename>" +
           filename + "</Filename></Exclude><Menu><Name>lume-chromium-apps</Name><Directory>" +
           kMenuDirectory + "</Directory><Include><Filename>" + filename +
           "</Filename></Include></Menu></Menu>\n";
}

bool ReadOwnedFragment(const Locations &paths, const Identity &identity)
{
    std::string contents;
    const base::FilePath path = FragmentPath(paths, identity);
    return !path.empty() && base::ReadFileToStringWithMaxSize(path, &contents, kEntryLimit) &&
           contents == FragmentContents(identity);
}

bool WriteMenu(const Locations &paths, const Identity &identity)
{
    if (!MenuSupported(paths) || !base::CreateDirectory(paths.directories) ||
        !base::CreateDirectory(paths.fragments))
        return false;
    const base::FilePath directory = paths.directories.Append(kMenuDirectory);
    if (base::PathExists(directory))
    {
        std::string contents;
        if (!base::ReadFileToStringWithMaxSize(directory, &contents, kEntryLimit) ||
            contents != kDirectoryContents)
            return false;
    }
    const base::FilePath fragment = FragmentPath(paths, identity);
    if (base::PathExists(fragment) && !ReadOwnedFragment(paths, identity))
        return false;
    return base::ImportantFileWriter::WriteFileAtomically(directory, kDirectoryContents) &&
           base::ImportantFileWriter::WriteFileAtomically(fragment, FragmentContents(identity));
}

bool DeleteMenu(const Locations &paths, const Identity &identity)
{
    const base::FilePath fragment = FragmentPath(paths, identity);
    return !fragment.empty() &&
           (!base::PathExists(fragment) ||
            (ReadOwnedFragment(paths, identity) && base::DeleteFile(fragment)));
}

base::FilePath EntryPath(const base::FilePath &directory, const Identity &identity)
{
    return directory.empty() ? base::FilePath()
                             : directory.AppendASCII(identity.Stem() + ".desktop");
}

bool ReadOwnedEntry(const base::FilePath &path, const Identity &identity, std::string *contents)
{
    return !path.empty() && base::ReadFileToStringWithMaxSize(path, contents, kEntryLimit) &&
           contents->starts_with("[Desktop Entry]\n") &&
           contents->find(identity.Marker()) != std::string::npos;
}

bool DeleteOwnedEntry(const base::FilePath &path, const Identity &identity)
{
    if (path.empty())
        return false;
    if (!base::PathExists(path))
        return true;
    std::string contents;
    return ReadOwnedEntry(path, identity, &contents) && base::DeleteFile(path);
}

bool AnyOwnedEntry(const Locations &paths, const Identity &identity)
{
    std::string contents;
    return ReadOwnedEntry(EntryPath(paths.desktop, identity), identity, &contents) ||
           ReadOwnedEntry(EntryPath(paths.applications, identity), identity, &contents) ||
           ReadOwnedEntry(EntryPath(paths.startup, identity), identity, &contents);
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

// Exec arguments use desktop-entry quoting before string-value escaping.
std::optional<std::string> QuoteExec(const base::CommandLine &command)
{
    std::string result;
    for (const auto &argument : command.argv())
    {
        if (argument.find_first_of("\n\r") != std::string::npos ||
            argument.find('\0') != std::string::npos)
            return std::nullopt;
        if (!result.empty())
            result += ' ';
        result += '"';
        for (const char character : argument)
        {
            if (character == '%')
                result += '%';
            if (character == '"' || character == '`' || character == '$' || character == '\\')
                result += '\\';
            result += character;
        }
        result += '"';
    }
    return EscapeValue(result);
}

bool WriteEntry(const base::FilePath &path, const Identity &identity, std::string_view contents)
{
    if (path.empty() || !base::CreateDirectory(path.DirName()))
        return false;
    if (base::PathExists(path))
    {
        std::string previous;
        if (!ReadOwnedEntry(path, identity, &previous))
            return false;
    }
    return base::ImportantFileWriter::WriteFileAtomically(path, contents) &&
           base::SetPosixFilePermissions(path, 0755);
}

std::string EntryContents(const ShortcutInfo &info, const Identity &identity,
                          const base::FilePath &icon, bool hidden, bool startup)
{
    base::CommandLine command = shell_integration::CommandLineArgsForLauncher(
        info.url, info.app_id, info.profile_path, startup ? kRunOnOsLoginModeWindowed : "");
    command.SetProgram(base::FilePath(kLauncher));
    const auto exec = QuoteExec(command);
    if (!exec)
        return {};
    const std::string name = info.title.empty() ? info.url.spec() : base::UTF16ToUTF8(info.title);
    if (name.find('\0') != std::string::npos)
        return {};
    std::string contents = "[Desktop Entry]\nType=Application\nVersion=1.0\nTerminal=false\nName=" +
                           EscapeValue(name) + "\nExec=" + *exec +
                           "\nIcon=" + EscapeValue(icon.value()) + "\n";
    if (hidden)
        contents += "NoDisplay=true\n";
    contents += identity.Marker();
    return contents.size() <= kEntryLimit ? contents : std::string();
}

ShortcutLocations FindExisting(const Locations &paths, const Identity &identity)
{
    ShortcutLocations existing;
    std::string contents;
    existing.on_desktop = ReadOwnedEntry(EntryPath(paths.desktop, identity), identity, &contents);
    existing.in_startup = ReadOwnedEntry(EntryPath(paths.startup, identity), identity, &contents);
    if (ReadOwnedEntry(EntryPath(paths.applications, identity), identity, &contents))
    {
        if (contents.find("\nNoDisplay=true\n") != std::string::npos)
            existing.applications_menu_location = APP_MENU_LOCATION_HIDDEN;
        else if (MenuSupported(paths) && ReadOwnedFragment(paths, identity))
            existing.applications_menu_location = APP_MENU_LOCATION_SUBDIR_CHROMEAPPS;
    }
    return existing;
}

bool HasLocations(const ShortcutLocations &locations)
{
    return locations.on_desktop || locations.in_startup ||
           locations.applications_menu_location != APP_MENU_LOCATION_NONE;
}

bool WriteShortcuts(const ShortcutInfo &info, const ShortcutLocations &requested)
{
    if (requested.in_quick_launch_bar || !HasLocations(requested) || access(kLauncher, X_OK) ||
        access(kChromium, X_OK))
        return false;
    const auto identity = MakeIdentity(info);
    if (!identity)
        return false;
    const Locations paths = GetLocations();
    if (requested.applications_menu_location == APP_MENU_LOCATION_SUBDIR_CHROMEAPPS &&
        !MenuSupported(paths))
        return false;
    for (const auto &[directory, enabled] :
         {std::pair(paths.desktop, requested.on_desktop),
          std::pair(paths.applications,
                    requested.applications_menu_location != APP_MENU_LOCATION_NONE),
          std::pair(paths.startup, requested.in_startup)})
    {
        if (!enabled)
            continue;
        const base::FilePath entry = EntryPath(directory, *identity);
        std::string contents;
        if (entry.empty() ||
            (base::PathExists(entry) && !ReadOwnedEntry(entry, *identity, &contents)))
            return false;
    }
    if (paths.icons.empty() || !base::CreateDirectory(paths.icons))
        return false;
    const base::FilePath icon = paths.icons.AppendASCII(identity->Stem() + ".png");
    scoped_refptr<base::RefCountedMemory> png;
    int width = 0;
    for (auto it = info.favicon.begin(); it != info.favicon.end(); ++it)
    {
        if (it->Width() > width)
        {
            auto candidate = it->As1xPNGBytes();
            if (candidate && candidate->size())
            {
                png = std::move(candidate);
                width = it->Width();
            }
        }
    }
    if (!png)
        return false;
    const bool existing = AnyOwnedEntry(paths, *identity);
    if (base::PathExists(icon) && !existing && !ReadOwnedFragment(paths, *identity))
        return false;
    const auto bytes = static_cast<base::span<const uint8_t>>(*png);
    const std::string_view image(reinterpret_cast<const char *>(bytes.data()), bytes.size());
    if (!base::ImportantFileWriter::WriteFileAtomically(icon, image))
        return false;
    if (requested.applications_menu_location == APP_MENU_LOCATION_SUBDIR_CHROMEAPPS &&
        !WriteMenu(paths, *identity))
    {
        if (!existing)
            base::DeleteFile(icon);
        return false;
    }
    bool success = true;
    for (const auto &[directory, enabled, hidden, startup] :
         {std::tuple(paths.desktop, requested.on_desktop, false, false),
          std::tuple(paths.applications,
                     requested.applications_menu_location != APP_MENU_LOCATION_NONE,
                     requested.applications_menu_location == APP_MENU_LOCATION_HIDDEN, false),
          std::tuple(paths.startup, requested.in_startup, false, true)})
    {
        if (!enabled)
            continue;
        const std::string contents = EntryContents(info, *identity, icon, hidden, startup);
        success = !contents.empty() &&
                  WriteEntry(EntryPath(directory, *identity), *identity, contents) && success;
    }
    if (requested.applications_menu_location == APP_MENU_LOCATION_HIDDEN)
        success = DeleteMenu(paths, *identity) && success;
    if (!success && !AnyOwnedEntry(paths, *identity))
    {
        DeleteMenu(paths, *identity);
        base::DeleteFile(icon);
    }
    return success;
}

bool DeleteShortcuts(const Locations &paths, const Identity &identity)
{
    bool success = true;
    bool owned = ReadOwnedFragment(paths, identity);
    for (const auto &directory : {paths.desktop, paths.applications, paths.startup})
    {
        const base::FilePath path = EntryPath(directory, identity);
        std::string contents;
        owned |= ReadOwnedEntry(path, identity, &contents);
        success = DeleteOwnedEntry(path, identity) && success;
    }
    success = DeleteMenu(paths, identity) && success;
    if (owned && !paths.icons.empty() && !AnyOwnedEntry(paths, identity))
        success = base::DeleteFile(paths.icons.AppendASCII(identity.Stem() + ".png")) && success;
    return success;
}

} // namespace

bool ShouldRegisterFileHandlersWithOs()
{
    return false;
}

void RegisterFileHandlersWithOs(const webapps::AppId &, const std::string &, const base::FilePath &,
                                const apps::FileHandlers &, ResultCallback callback)
{
    std::move(callback).Run(Result::kError);
}

void UnregisterFileHandlersWithOs(const webapps::AppId &, const base::FilePath &,
                                  ResultCallback callback)
{
    std::move(callback).Run(Result::kError);
}

namespace internals
{

void CreatePlatformShortcuts(const base::FilePath &, const ShortcutLocations &locations,
                             ShortcutCreationReason, const ShortcutInfo &info,
                             CreateShortcutsCallback callback)
{
    base::ScopedBlockingCall blocking(FROM_HERE, base::BlockingType::MAY_BLOCK);
    std::move(callback).Run(WriteShortcuts(info, locations));
}

ShortcutLocations GetAppExistingShortCutLocationImpl(const ShortcutInfo &info)
{
    base::ScopedBlockingCall blocking(FROM_HERE, base::BlockingType::MAY_BLOCK);
    const auto identity = MakeIdentity(info);
    return identity ? FindExisting(GetLocations(), *identity) : ShortcutLocations();
}

void DeletePlatformShortcuts(const base::FilePath &, const ShortcutInfo &info,
                             scoped_refptr<base::TaskRunner> runner,
                             DeleteShortcutsCallback callback)
{
    base::ScopedBlockingCall blocking(FROM_HERE, base::BlockingType::MAY_BLOCK);
    const auto identity = MakeIdentity(info);
    const bool success = identity && DeleteShortcuts(GetLocations(), *identity);
    runner->PostTask(FROM_HERE, base::BindOnce(std::move(callback), success));
}

void UpdatePlatformShortcuts(const base::FilePath &, const std::u16string &,
                             std::optional<ShortcutLocations> locations, ResultCallback callback,
                             const ShortcutInfo &info)
{
    base::ScopedBlockingCall blocking(FROM_HERE, base::BlockingType::MAY_BLOCK);
    ShortcutLocations requested = GetAppExistingShortCutLocationImpl(info);
    if (locations)
        requested = MergeLocations(*locations, requested);
    std::move(callback).Run(WriteShortcuts(info, requested) ? Result::kOk : Result::kError);
}

void DeleteAllShortcutsForProfile(const base::FilePath &profile)
{
    base::ScopedBlockingCall blocking(FROM_HERE, base::BlockingType::MAY_BLOCK);
    const auto identity = MakeIdentity(profile, "profile");
    if (!identity)
        return;
    const Locations paths = GetLocations();
    for (const auto &directory : {paths.desktop, paths.applications, paths.startup})
    {
        if (directory.empty())
            continue;
        base::FileEnumerator files(directory, false, base::FileEnumerator::FILES,
                                   identity->Prefix() + "*.desktop");
        for (base::FilePath path = files.Next(); !path.empty(); path = files.Next())
        {
            const std::string name = path.BaseName().RemoveExtension().value();
            const std::string app = name.substr(identity->Prefix().size());
            if (app.size() != 40 || app.find_first_not_of("0123456789ABCDEF") != std::string::npos)
                continue;
            const Identity entry{identity->profile, app};
            std::string contents;
            if (ReadOwnedEntry(path, entry, &contents) && !DeleteShortcuts(paths, entry))
                LOG(ERROR) << "Unable to remove web app shortcuts";
        }
    }
    if (!paths.fragments.empty())
    {
        base::FileEnumerator files(paths.fragments, false, base::FileEnumerator::FILES,
                                   identity->Prefix() + "*.menu");
        for (base::FilePath path = files.Next(); !path.empty(); path = files.Next())
        {
            const std::string name = path.BaseName().RemoveExtension().value();
            const std::string app = name.substr(identity->Prefix().size());
            if (app.size() != 40 || app.find_first_not_of("0123456789ABCDEF") != std::string::npos)
                continue;
            const Identity entry{identity->profile, app};
            if (ReadOwnedFragment(paths, entry) && !DeleteShortcuts(paths, entry))
                LOG(ERROR) << "Unable to remove web app menu registration";
        }
    }
}

void RegisterRunOnOsLogin(const ShortcutInfo &info, ResultCallback callback)
{
    base::ScopedBlockingCall blocking(FROM_HERE, base::BlockingType::MAY_BLOCK);
    ShortcutLocations locations;
    locations.in_startup = true;
    std::move(callback).Run(WriteShortcuts(info, locations) ? Result::kOk : Result::kError);
}

Result UnregisterRunOnOsLogin(const std::string &app_id, const base::FilePath &profile,
                              const std::u16string &)
{
    base::ScopedBlockingCall blocking(FROM_HERE, base::BlockingType::MAY_BLOCK);
    ShortcutInfo info;
    info.profile_path = profile;
    info.app_id = app_id;
    const auto identity = MakeIdentity(info);
    if (!identity)
        return Result::kError;
    const Locations paths = GetLocations();
    const base::FilePath path = EntryPath(paths.startup, *identity);
    std::string contents;
    const bool owned = ReadOwnedEntry(path, *identity, &contents);
    bool success = DeleteOwnedEntry(path, *identity);
    if (owned && !AnyOwnedEntry(paths, *identity) && !paths.icons.empty())
        success = base::DeleteFile(paths.icons.AppendASCII(identity->Stem() + ".png")) && success;
    return success ? Result::kOk : Result::kError;
}

} // namespace internals
} // namespace web_app
