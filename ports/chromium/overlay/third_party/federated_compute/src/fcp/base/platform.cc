/*
 * Copyright 2017 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "fcp/base/platform.h"

#if defined(__lume__)
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <array>
#endif

#include <stdlib.h>
#include <sys/stat.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include "absl/status/status.h"

#ifdef _WIN32
#include <direct.h>
#endif

#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "fcp/base/monitoring.h"

namespace fcp
{

namespace
{
#ifdef _WIN32
constexpr char kPathSeparator[] = "\\";
#else
constexpr char kPathSeparator[] = "/";
#endif
#if defined(__lume__)
void AppendFileChunk(std::string *output, absl::string_view chunk)
{
    output->append(chunk.data(), chunk.size());
}

void AppendFileChunk(absl::Cord *output, absl::string_view chunk)
{
    output->Append(chunk);
}

bool WriteAll(int fd, absl::string_view content)
{
    while (!content.empty())
    {
        const size_t count = content.size() < 8192 ? content.size() : 8192;
        const ssize_t written = write(fd, content.data(), count);
        if (written < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return false;
        }
        if (written == 0 || static_cast<size_t>(written) > count)
        {
            return false;
        }
        content.remove_prefix(static_cast<size_t>(written));
    }
    return true;
}

template <typename Writer>
absl::Status WriteFileContents(absl::string_view file_name, Writer writer)
{
    const std::string file_name_str(file_name);
    const int fd = open(file_name_str.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0666);
    if (fd < 0)
    {
        return absl::InternalError(absl::StrCat("cannot create file ", file_name_str));
    }
    const bool written = writer(fd);
    const int closed = close(fd);
    if (!written || closed != 0)
    {
        return absl::InternalError(absl::StrCat("error writing to file ", file_name_str));
    }
    return absl::OkStatus();
}
#endif

} // namespace

std::string ConcatPath(absl::string_view path1, absl::string_view path2)
{
    if (path1.empty())
    {
        return std::string(path2);
    }
    return absl::StrCat(path1, kPathSeparator, path2);
}

absl::string_view StripTrailingPathSeparator(absl::string_view path)
{
    return absl::StripSuffix(path, kPathSeparator);
}

namespace internal
{

template <typename T> absl::StatusOr<T> ReadFile(absl::string_view file_name)
{
    auto file_name_str = std::string(file_name);
#if defined(__lume__)
    const int fd = open(file_name_str.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0)
    {
        return absl::InternalError(absl::StrCat("cannot read file ", file_name_str));
    }
    T content;
    std::array<char, 8192> buffer;
    bool read_ok = true;
    for (;;)
    {
        const ssize_t count = read(fd, buffer.data(), buffer.size());
        if (count < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            read_ok = false;
            break;
        }
        if (count == 0)
        {
            break;
        }
        if (static_cast<size_t>(count) > buffer.size())
        {
            read_ok = false;
            break;
        }
        AppendFileChunk(&content, absl::string_view(buffer.data(), static_cast<size_t>(count)));
    }
    const int closed = close(fd);
    if (!read_ok || closed != 0)
    {
        return absl::InternalError(absl::StrCat("error reading file ", file_name_str));
    }
    return content;
#else
    std::ifstream is(file_name_str);
    if (!is)
    {
        return absl::InternalError(absl::StrCat("cannot read file ", file_name_str));
    }
    std::ostringstream buffer;
    buffer << is.rdbuf();
    if (!is)
    {
        return absl::InternalError(absl::StrCat("error reading file ", file_name_str));
    }
    return static_cast<T>(buffer.str());
#endif
}

} // namespace internal

absl::StatusOr<std::string> ReadFileToString(absl::string_view file_name)
{
    return internal::ReadFile<std::string>(file_name);
}

absl::StatusOr<absl::Cord> ReadFileToCord(absl::string_view file_name)
{
    return internal::ReadFile<absl::Cord>(file_name);
}

absl::Status WriteStringToFile(absl::string_view file_name, absl::string_view content)
{
#if defined(__lume__)
    return WriteFileContents(file_name,
                             [content](int fd)
                             {
                                 return WriteAll(fd, content);
                             });
#else
    auto file_name_str = std::string(file_name);
    std::ofstream os(file_name_str);
    if (!os)
    {
        return absl::InternalError(absl::StrCat("cannot create file ", file_name_str));
    }
    os << content;
    if (!os)
    {
        return absl::InternalError(absl::StrCat("error writing to file ", file_name_str));
    }
    return absl::OkStatus();
#endif
}

absl::Status WriteCordToFile(absl::string_view file_name, const absl::Cord &content)
{
#if defined(__lume__)
    return WriteFileContents(file_name,
                             [&content](int fd)
                             {
                                 for (absl::string_view chunk : content.Chunks())
                                 {
                                     if (!WriteAll(fd, chunk))
                                     {
                                         return false;
                                     }
                                 }
                                 return true;
                             });
#else
    auto file_name_str = std::string(file_name);
    std::ofstream os(file_name_str);
    if (!os)
    {
        return absl::InternalError(absl::StrCat("cannot create file ", file_name_str));
    }
    for (absl::string_view chunk : content.Chunks())
    {
        os << chunk;
        if (!os)
        {
            return absl::InternalError(absl::StrCat("error writing to file ", file_name_str));
        }
    }
    return absl::OkStatus();
#endif
}

bool FileExists(absl::string_view file_name)
{
    struct stat info;
    return stat(std::string(file_name).c_str(), &info) == 0;
}

std::string GetDataPath(absl::string_view relative_path)
{
    return std::string(relative_path);
}

} // namespace fcp
