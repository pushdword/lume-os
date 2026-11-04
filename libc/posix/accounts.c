/* SPDX-License-Identifier: BSD-3-Clause */
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

static struct passwd root = {.pw_name = "root",
                             .pw_passwd = "*",
                             .pw_uid = 0,
                             .pw_gid = 0,
                             .pw_comment = "Administrator",
                             .pw_gecos = "Administrator",
                             .pw_dir = "/data/home",
                             .pw_shell = "/bin/sh"};

struct passwd *getpwuid(uid_t uid)
{
    return uid == 0 ? &root : NULL;
}

struct passwd *getpwnam(const char *name)
{
    return name && !strcmp(name, root.pw_name) ? &root : NULL;
}

static unsigned account_cursor;

void setpwent(void)
{
    account_cursor = 0;
}

struct passwd *getpwent(void)
{
    if (account_cursor)
        return NULL;
    account_cursor = 1;
    return &root;
}

void endpwent(void)
{
    account_cursor = 1;
}

static int copy_account(struct passwd *entry, struct passwd *out, char *buffer, size_t bytes,
                        struct passwd **result)
{
    *result = NULL;
    if (!entry)
        return 0;
    char **target[] = {&out->pw_name,  &out->pw_passwd, &out->pw_comment,
                       &out->pw_gecos, &out->pw_dir,    &out->pw_shell};
    const char *source[] = {entry->pw_name,  entry->pw_passwd, entry->pw_comment,
                            entry->pw_gecos, entry->pw_dir,    entry->pw_shell};
    *out = *entry;
    for (unsigned i = 0; i < sizeof(source) / sizeof(source[0]); ++i)
    {
        size_t size = strlen(source[i]) + 1;
        if (size > bytes)
            return ERANGE;
        memcpy(buffer, source[i], size);
        *target[i] = buffer;
        buffer += size;
        bytes -= size;
    }
    *result = out;
    return 0;
}

int getpwuid_r(uid_t uid, struct passwd *out, char *buffer, size_t bytes, struct passwd **result)
{
    return copy_account(getpwuid(uid), out, buffer, bytes, result);
}

int getpwnam_r(const char *name, struct passwd *out, char *buffer, size_t bytes,
               struct passwd **result)
{
    return copy_account(getpwnam(name), out, buffer, bytes, result);
}

char *getlogin(void)
{
    struct passwd *entry = getpwuid(getuid());
    if (!entry)
        errno = ENOENT;
    return entry ? entry->pw_name : NULL;
}

struct group *getgrgid(gid_t gid)
{
    static char *members[] = {"root", NULL};
    static struct group group = {
        .gr_name = "root", .gr_passwd = "*", .gr_gid = 0, .gr_mem = members};
    return gid == 0 ? &group : NULL;
}

struct group *getgrnam(const char *name)
{
    return name && !strcmp(name, "root") ? getgrgid(0) : NULL;
}

static unsigned group_cursor;

void setgrent(void)
{
    group_cursor = 0;
}

struct group *getgrent(void)
{
    if (group_cursor)
        return NULL;
    group_cursor = 1;
    return getgrgid(0);
}

void endgrent(void)
{
    group_cursor = 1;
}

static int copy_group(const struct group *entry, struct group *out, char *buffer, size_t bytes,
                      struct group **result)
{
    *result = NULL;
    if (!entry)
        return 0;
    size_t members = 0;
    while (entry->gr_mem[members])
        ++members;
    size_t padding = (-(uintptr_t)buffer) & (sizeof(char *) - 1);
    size_t name_bytes = strlen(entry->gr_name) + 1;
    size_t password_bytes = strlen(entry->gr_passwd) + 1;
    size_t pointers = (members + 1) * sizeof(char *);
    size_t required = padding + pointers + name_bytes + password_bytes;
    for (size_t i = 0; i < members; ++i)
        required += strlen(entry->gr_mem[i]) + 1;
    if (required > bytes)
        return ERANGE;
    struct group copy = *entry;
    copy.gr_mem = (char **)(buffer + padding);
    char *cursor = buffer + padding + pointers;
    memcpy(cursor, entry->gr_name, name_bytes);
    copy.gr_name = cursor;
    cursor += name_bytes;
    memcpy(cursor, entry->gr_passwd, password_bytes);
    copy.gr_passwd = cursor;
    cursor += password_bytes;
    for (size_t i = 0; i < members; ++i)
    {
        size_t length = strlen(entry->gr_mem[i]) + 1;
        memcpy(cursor, entry->gr_mem[i], length);
        copy.gr_mem[i] = cursor;
        cursor += length;
    }
    copy.gr_mem[members] = NULL;
    *out = copy;
    *result = out;
    return 0;
}

int getgrgid_r(gid_t gid, struct group *out, char *buffer, size_t bytes, struct group **result)
{
    return copy_group(getgrgid(gid), out, buffer, bytes, result);
}

int getgrnam_r(const char *name, struct group *out, char *buffer, size_t bytes,
               struct group **result)
{
    return copy_group(getgrnam(name), out, buffer, bytes, result);
}
