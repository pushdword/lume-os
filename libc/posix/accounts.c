/* SPDX-License-Identifier: BSD-3-Clause */
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

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
