/* SPDX-License-Identifier: BSD-3-Clause */
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include "lume/credential.h"
#include "accounts_database.h"

static const struct passwd root = {.pw_name = "root",
                                   .pw_passwd = "*",
                                   .pw_uid = 0,
                                   .pw_gid = 0,
                                   .pw_comment = "Administrator",
                                   .pw_gecos = "Administrator",
                                   .pw_dir = "/data/home",
                                   .pw_shell = "/bin/sh"};

static int bootstrap_account(uid_t, struct passwd *, char *, size_t, struct passwd **);

static char account_buffer[4096], group_buffer[4096];
static struct passwd account;
static struct group account_group;

struct passwd *getpwuid(uid_t uid)
{
    struct passwd *result;
    int error = getpwuid_r(uid, &account, account_buffer, sizeof(account_buffer), &result);
    if (error)
        errno = error;
    return result;
}

struct passwd *getpwnam(const char *name)
{
    struct passwd *result;
    int error = getpwnam_r(name, &account, account_buffer, sizeof(account_buffer), &result);
    if (error)
        errno = error;
    return result;
}

static unsigned account_cursor;
static FILE *account_stream;

void setpwent(void)
{
    endpwent();
    account_cursor = 0;
}

struct passwd *getpwent(void)
{
    if (account_cursor)
        return NULL;
    if (!account_stream)
    {
        account_stream = fopen(LUME_PASSWD_PATH, "r");
        if (!account_stream)
        {
            account_cursor = 1;
            if (errno != ENOENT)
                return NULL;
            struct passwd *result;
            int error =
                bootstrap_account(0, &account, account_buffer, sizeof(account_buffer), &result);
            if (error)
                errno = error;
            return result;
        }
    }
    int result =
        account_record(account_stream, false, &account, account_buffer, sizeof(account_buffer));
    if (result > 0)
        return &account;
    endpwent();
    account_cursor = 1;
    if (result < 0)
        errno = -result;
    return NULL;
}

void endpwent(void)
{
    int previous = errno;
    if (!account_stream || !fclose(account_stream))
        errno = previous;
    account_stream = NULL;
    account_cursor = 0;
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

static int bootstrap_account(uid_t uid, struct passwd *out, char *buffer, size_t bytes,
                             struct passwd **result)
{
    struct passwd entry = root;
    entry.pw_shell = access("/data/bin/bash", X_OK) ? "/bin/sh" : "/data/bin/bash";
    return copy_account(uid == 0 ? &entry : NULL, out, buffer, bytes, result);
}

int getpwuid_r(uid_t uid, struct passwd *out, char *buffer, size_t bytes, struct passwd **result)
{
    if (!result)
        return EINVAL;
    *result = NULL;
    bool missing;
    int found = account_find(false, NULL, (uint32_t)uid, out, buffer, bytes, &missing);
    if (found < 0)
        return -found;
    if (missing)
        return bootstrap_account(uid, out, buffer, bytes, result);
    *result = found ? out : NULL;
    return 0;
}

int getpwnam_r(const char *name, struct passwd *out, char *buffer, size_t bytes,
               struct passwd **result)
{
    if (!result)
        return EINVAL;
    *result = NULL;
    if (!name)
        return EINVAL;
    bool missing;
    int found = account_find(false, name, 0, out, buffer, bytes, &missing);
    if (found < 0)
        return -found;
    if (missing)
        return bootstrap_account(!strcmp(name, "root") ? 0 : (uid_t)-1, out, buffer, bytes, result);
    *result = found ? out : NULL;
    return 0;
}

char *getlogin(void)
{
    struct passwd *entry = getpwuid(getuid());
    if (!entry)
        errno = ENOENT;
    return entry ? entry->pw_name : NULL;
}

static struct group *bootstrap_group(gid_t gid)
{
    static char *members[] = {"root", NULL};
    static struct group group = {
        .gr_name = "root", .gr_passwd = "*", .gr_gid = 0, .gr_mem = members};
    return gid == 0 ? &group : NULL;
}

struct group *getgrgid(gid_t gid)
{
    struct group *result;
    int error = getgrgid_r(gid, &account_group, group_buffer, sizeof(group_buffer), &result);
    if (error)
        errno = error;
    return result;
}

struct group *getgrnam(const char *name)
{
    struct group *result;
    int error = getgrnam_r(name, &account_group, group_buffer, sizeof(group_buffer), &result);
    if (error)
        errno = error;
    return result;
}

static unsigned group_cursor;
static FILE *group_stream;

void setgrent(void)
{
    endgrent();
    group_cursor = 0;
}

struct group *getgrent(void)
{
    if (group_cursor)
        return NULL;
    if (!group_stream)
    {
        group_stream = fopen(LUME_GROUP_PATH, "r");
        if (!group_stream)
        {
            group_cursor = 1;
            return errno == ENOENT ? bootstrap_group(0) : NULL;
        }
    }
    int result =
        account_record(group_stream, true, &account_group, group_buffer, sizeof(group_buffer));
    if (result > 0)
        return &account_group;
    endgrent();
    group_cursor = 1;
    if (result < 0)
        errno = -result;
    return NULL;
}

void endgrent(void)
{
    int previous = errno;
    if (!group_stream || !fclose(group_stream))
        errno = previous;
    group_stream = NULL;
    group_cursor = 0;
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
    if (!result)
        return EINVAL;
    *result = NULL;
    bool missing;
    int found = account_find(true, NULL, (uint32_t)gid, out, buffer, bytes, &missing);
    if (found < 0)
        return -found;
    if (missing)
        return copy_group(bootstrap_group(gid), out, buffer, bytes, result);
    *result = found ? out : NULL;
    return 0;
}

int getgrnam_r(const char *name, struct group *out, char *buffer, size_t bytes,
               struct group **result)
{
    if (!result)
        return EINVAL;
    *result = NULL;
    if (!name)
        return EINVAL;
    bool missing;
    int found = account_find(true, name, 0, out, buffer, bytes, &missing);
    if (found < 0)
        return -found;
    if (missing)
        return copy_group(!strcmp(name, "root") ? bootstrap_group(0) : NULL, out, buffer, bytes,
                          result);
    *result = found ? out : NULL;
    return 0;
}

int initgroups(const char *name, gid_t primary)
{
    if (!name || !*name || primary > LUME_ID_MAX)
    {
        errno = EINVAL;
        return -1;
    }
    gid_t groups[LUME_GROUP_MAX] = {primary};
    unsigned count = 1;
    FILE *stream = fopen(LUME_GROUP_PATH, "r");
    if (!stream)
        return errno == ENOENT ? setgroups((int)count, groups) : -1;
    char buffer[4096];
    struct group entry;
    int result;
    while ((result = account_record(stream, true, &entry, buffer, sizeof(buffer))) > 0)
    {
        bool member = false;
        for (char **user = entry.gr_mem; !member && *user; ++user)
            member = !strcmp(name, *user);
        if (!member)
            continue;
        unsigned index = 0;
        while (index < count && groups[index] != entry.gr_gid)
            ++index;
        if (index != count)
            continue;
        if (count == LUME_GROUP_MAX || entry.gr_gid > LUME_ID_MAX)
        {
            result = -EINVAL;
            break;
        }
        groups[count++] = entry.gr_gid;
    }
    fclose(stream);
    if (result < 0)
    {
        errno = -result;
        return -1;
    }
    return setgroups((int)count, groups);
}
