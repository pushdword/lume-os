/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_ACCOUNTS_DATABASE_H
#define LUME_ACCOUNTS_DATABASE_H
#include <stdbool.h>
#include <stdio.h>
#include <limits.h>

#ifndef LUME_PASSWD_PATH
#define LUME_PASSWD_PATH "/etc/passwd"
#endif
#ifndef LUME_GROUP_PATH
#define LUME_GROUP_PATH "/etc/group"
#endif

static bool account_id(const char *text, uint32_t *out)
{
    if (!*text)
        return false;
    uint32_t value = 0;
    for (; *text; ++text)
    {
        unsigned digit = (unsigned char)*text - '0';
        if (digit > 9 || value > (UINT32_MAX - digit) / 10)
            return false;
        value = value * 10 + digit;
    }
    *out = value;
    return true;
}

static int account_line(FILE *stream, char *buffer, size_t bytes)
{
    if (bytes < 2)
        return -ERANGE;
    if (!fgets(buffer, bytes > INT_MAX ? INT_MAX : (int)bytes, stream))
        return ferror(stream) ? -(errno ? errno : EIO) : 0;
    size_t length = strlen(buffer);
    if (length && buffer[length - 1] == '\n')
        buffer[--length] = 0;
    else if (!feof(stream))
    {
        if (fgetc(stream) != EOF)
            return -ERANGE;
        if (ferror(stream))
            return -(errno ? errno : EIO);
    }
    if (length && buffer[length - 1] == '\r')
        buffer[length - 1] = 0;
    return 1;
}

static bool account_fields(char *line, char **fields, unsigned count)
{
    if (!line[0] || line[0] == '#')
        return false;
    for (unsigned i = 0; i < count; ++i)
    {
        fields[i] = line;
        char *separator = strchr(line, ':');
        if (i + 1 == count)
            return !separator;
        if (!separator)
            return false;
        *separator = 0;
        line = separator + 1;
    }
    return false;
}

static int account_record(FILE *stream, bool is_group, void *out, char *buffer, size_t bytes)
{
    for (;;)
    {
        int result = account_line(stream, buffer, bytes);
        if (result <= 0)
            return result;
        size_t used = strlen(buffer) + 1;
        char *field[7];
        uint32_t uid = 0, gid;
        if (!account_fields(buffer, field, is_group ? 4 : 7) || !field[0][0] ||
            !account_id(field[is_group ? 2 : 3], &gid) || (uint32_t)(gid_t)gid != gid ||
            (gid_t)gid == (gid_t)-1)
            continue;
        if (!is_group)
        {
            if (!account_id(field[2], &uid) || (uint32_t)(uid_t)uid != uid ||
                (uid_t)uid == (uid_t)-1)
                continue;
            *(struct passwd *)out = (struct passwd){.pw_name = field[0],
                                                    .pw_passwd = field[1],
                                                    .pw_uid = (uid_t)uid,
                                                    .pw_gid = (gid_t)gid,
                                                    .pw_comment = field[4],
                                                    .pw_gecos = field[4],
                                                    .pw_dir = field[5],
                                                    .pw_shell = field[6]};
            return 1;
        }
        size_t count = field[3][0] != 0;
        for (const char *p = field[3]; *p; ++p)
            count += *p == ',';
        size_t padding = ((uintptr_t)0 - (uintptr_t)(buffer + used)) & (_Alignof(char *) - 1);
        if (padding > bytes - used || count >= (bytes - used - padding) / sizeof(char *))
            return -ERANGE;
        char **members = (char **)(buffer + used + padding);
        char *member = field[3];
        bool valid = true;
        for (size_t i = 0; i < count; ++i)
        {
            members[i] = member;
            char *separator = strchr(member, ',');
            if (separator)
                *separator = 0;
            valid &= *member != 0;
            member = separator ? separator + 1 : member + strlen(member);
        }
        if (!valid)
            continue;
        members[count] = NULL;
        *(struct group *)out = (struct group){
            .gr_name = field[0], .gr_passwd = field[1], .gr_gid = (gid_t)gid, .gr_mem = members};
        return 1;
    }
}

/* Lookups own their buffered stream and never change enumeration state.
 * A missing file enables bootstrap records. Read and permission errors propagate. */
static int account_find(bool is_group, const char *name, uint32_t id, void *out, char *buffer,
                        size_t bytes, bool *missing)
{
    *missing = false;
    if (!out || !buffer)
        return -EINVAL;
    FILE *stream = fopen(is_group ? LUME_GROUP_PATH : LUME_PASSWD_PATH, "r");
    if (!stream)
    {
        *missing = errno == ENOENT;
        return *missing ? 0 : -(errno ? errno : EIO);
    }
    int result;
    while ((result = account_record(stream, is_group, out, buffer, bytes)) > 0)
    {
        const char *found_name =
            is_group ? ((struct group *)out)->gr_name : ((struct passwd *)out)->pw_name;
        uint32_t found_id = is_group ? (uint32_t)((struct group *)out)->gr_gid
                                     : (uint32_t)((struct passwd *)out)->pw_uid;
        if (name ? !strcmp(name, found_name) : id == found_id)
            break;
    }
    fclose(stream);
    return result;
}
#endif
