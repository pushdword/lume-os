/* SPDX-License-Identifier: BSD-3-Clause */
#include "sudo_policy.h"
#include "auth_files.h"
#include "password.h"
#include "lume/abi.h"
#include "lume/credential.h"
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static bool valid_command_path(const char *path)
{
    size_t length = strlen(path);
    if (length < 2 || length >= LUME_PATH_MAX || path[0] != '/')
        return false;
    for (const char *part = path + 1; *part;)
    {
        size_t bytes = strcspn(part, "/");
        if (!bytes || (bytes == 1 && part[0] == '.') ||
            (bytes == 2 && part[0] == '.' && part[1] == '.'))
            return false;
        for (size_t i = 0; i < bytes; ++i)
            if ((unsigned char)part[i] <= ' ' || strchr("*?[]\\,#!", part[i]))
                return false;
        part += bytes;
        if (*part && !*++part)
            return false;
    }
    return true;
}

bool sudo_trusted_command(const char *path)
{
    if (!valid_command_path(path))
        return false;
    char components[LUME_PATH_MAX];
    strcpy(components, path + 1);
    int directory = auth_trusted_directory(AT_FDCWD, "/", false, 0755);
    char *part = components;
    for (char *slash; directory >= 0 && (slash = strchr(part, '/')); part = slash + 1)
    {
        *slash = 0;
        int next = auth_trusted_directory(directory, part, false, 0755);
        close(directory);
        directory = next;
    }
    if (directory < 0)
        return false;
    /* Unprivileged users cannot replace a component of this path after validation. */
    struct stat info;
    bool trusted = !fstatat(directory, part, &info, AT_SYMLINK_NOFOLLOW) && S_ISREG(info.st_mode) &&
                   !info.st_uid && !(info.st_mode & 022u) && (info.st_mode & 0111u);
    close(directory);
    return trusted;
}

static int read_policy(char *buffer, size_t capacity)
{
    int data = auth_trusted_directory(AT_FDCWD, "/data", false, 0755);
    if (data < 0)
        return -1;
    int etc = auth_trusted_directory(data, "etc", false, 0755);
    close(data);
    if (etc < 0)
        return -1;
    int file = openat(etc, "sudoers", O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC);
    close(etc);
    if (file < 0)
        return -1;
    struct stat info;
    if (fstat(file, &info) || !S_ISREG(info.st_mode) || info.st_uid || (info.st_mode & 022u) ||
        info.st_size < 0 || (uint64_t)info.st_size >= capacity)
    {
        close(file);
        return -1;
    }
    size_t bytes = 0;
    while (bytes < capacity)
    {
        ssize_t got = read(file, buffer + bytes, capacity - bytes);
        if (got > 0)
            bytes += (size_t)got;
        else if (!got)
            break;
        else if (errno != EINTR)
        {
            close(file);
            return -1;
        }
    }
    close(file);
    if (bytes != (uint64_t)info.st_size || memchr(buffer, 0, bytes))
        return -1;
    buffer[bytes] = 0;
    return 0;
}

static bool group_member(const char *name, gid_t group)
{
    struct group entry, *found;
    char buffer[4096];
    if (getgrnam_r(name, &entry, buffer, sizeof(buffer), &found) || !found)
        return false;
    if (entry.gr_gid == group)
        return true;
    gid_t groups[LUME_GROUP_MAX];
    int count = getgroups(LUME_GROUP_MAX, groups);
    for (int i = 0; i < count; ++i)
        if (groups[i] == entry.gr_gid)
            return true;
    return false;
}

/* A rule names one account or group, one command and optional literal arguments. */
static int match_rule(char *line, const char *name, gid_t group, char *const command[])
{
    static const char spaces[] = " \t\r";
    char *state;
    char *subject = strtok_r(line, spaces, &state);
    if (!subject || subject[0] == '#')
        return 0;
    bool in_group = subject[0] == '%';
    char *run_as = strtok_r(NULL, spaces, &state);
    char *program = strtok_r(NULL, spaces, &state);
    if (!auth_name_valid(subject + in_group) || !run_as || strcmp(run_as, "ALL=(root)") ||
        !program || (strcmp(program, "ALL") && !valid_command_path(program)))
        return -1;
    bool matches = !strcmp(program, "ALL") || !strcmp(program, command[0]);
    char *argument = strtok_r(NULL, spaces, &state);
    if (argument && !strcmp(argument, "\"\""))
    {
        if (strtok_r(NULL, spaces, &state))
            return -1;
        matches = matches && !command[1];
    }
    else if (argument)
    {
        char *const *actual = command + 1;
        for (; argument; argument = strtok_r(NULL, spaces, &state))
        {
            if (strpbrk(argument, "*?[]\\\"',!"))
                return -1;
            if (!*actual || strcmp(argument, *actual))
                matches = false;
            if (*actual)
                ++actual;
        }
        matches = matches && !*actual;
    }
    return matches && (in_group ? group_member(subject + 1, group) : !strcmp(subject, name));
}

bool sudo_policy_allows(const char *name, gid_t group, char *const command[])
{
    char contents[16385];
    if (read_policy(contents, sizeof(contents)))
        return false;
    bool allowed = false;
    char *state;
    for (char *line = strtok_r(contents, "\n", &state); line; line = strtok_r(NULL, "\n", &state))
    {
        int matched = match_rule(line, name, group, command);
        if (matched < 0)
            return false;
        allowed = allowed || matched;
    }
    return allowed;
}
