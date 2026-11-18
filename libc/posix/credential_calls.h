#ifndef LUME_CREDENTIAL_CALLS_H
#define LUME_CREDENTIAL_CALLS_H
#include "lume/credential.h"

static struct lume_credential_request identity_request(void)
{
    return (struct lume_credential_request){.version = LUME_CREDENTIAL_VERSION,
                                            .bytes = sizeof(struct lume_credential_request),
                                            .uid = UINT32_MAX,
                                            .euid = UINT32_MAX,
                                            .suid = UINT32_MAX,
                                            .gid = UINT32_MAX,
                                            .egid = UINT32_MAX,
                                            .sgid = UINT32_MAX};
}

static int credential(unsigned op, struct lume_credential_request *request)
{
    return native_result(
        native_call(LUME_SYS_CREDENTIAL, op, (uintptr_t)request, sizeof(*request)));
}

int setresuid(uid_t real, uid_t effective, uid_t saved)
{
    struct lume_credential_request r = identity_request();
    r.uid = real == (uid_t)-1 ? UINT32_MAX : real;
    r.euid = effective == (uid_t)-1 ? UINT32_MAX : effective;
    r.suid = saved == (uid_t)-1 ? UINT32_MAX : saved;
    return credential(LUME_CREDENTIAL_UID, &r);
}

int setresgid(gid_t real, gid_t effective, gid_t saved)
{
    struct lume_credential_request r = identity_request();
    r.gid = real == (gid_t)-1 ? UINT32_MAX : real;
    r.egid = effective == (gid_t)-1 ? UINT32_MAX : effective;
    r.sgid = saved == (gid_t)-1 ? UINT32_MAX : saved;
    return credential(LUME_CREDENTIAL_GID, &r);
}

int seteuid(uid_t uid)
{
    struct lume_credential_request r = identity_request();
    r.uid = uid;
    return credential(LUME_CREDENTIAL_SETEUID, &r);
}

int setegid(gid_t gid)
{
    struct lume_credential_request r = identity_request();
    r.gid = gid;
    return credential(LUME_CREDENTIAL_SETEGID, &r);
}

int setreuid(uid_t a, uid_t b)
{
    struct lume_credential_request r = identity_request();
    r.uid = a == (uid_t)-1 ? UINT32_MAX : a;
    r.euid = b == (uid_t)-1 ? UINT32_MAX : b;
    return credential(LUME_CREDENTIAL_SETREUID, &r);
}

int setregid(gid_t a, gid_t b)
{
    struct lume_credential_request r = identity_request();
    r.gid = a == (gid_t)-1 ? UINT32_MAX : a;
    r.egid = b == (gid_t)-1 ? UINT32_MAX : b;
    return credential(LUME_CREDENTIAL_SETREGID, &r);
}

int setuid(uid_t uid)
{
    struct lume_credential_request r = identity_request();
    r.uid = uid;
    return credential(LUME_CREDENTIAL_SETUID, &r);
}

int setgid(gid_t gid)
{
    struct lume_credential_request r = identity_request();
    r.gid = gid;
    return credential(LUME_CREDENTIAL_SETGID, &r);
}

int setgroups(int count, const gid_t *groups)
{
    struct lume_credential_request r = identity_request();
    if (count < 0 || (unsigned)count > LUME_GROUP_MAX)
    {
        errno = EINVAL;
        return -1;
    }
    if (count && !groups)
    {
        errno = EFAULT;
        return -1;
    }
    r.groups_count = count;
    for (int i = 0; i < count; ++i)
        r.groups[i] = groups[i];
    return credential(LUME_CREDENTIAL_GROUPS, &r);
}

int getgroups(int count, gid_t *groups)
{
    struct lume_credential_request r = identity_request();
    if (count < 0)
    {
        errno = EINVAL;
        return -1;
    }
    if (credential(LUME_CREDENTIAL_GET, &r))
        return -1;
    if (count && count < (int)r.groups_count)
    {
        errno = EINVAL;
        return -1;
    }
    if (count && r.groups_count && !groups)
    {
        errno = EFAULT;
        return -1;
    }
    for (unsigned i = 0; count && i < r.groups_count; ++i)
        groups[i] = r.groups[i];
    return r.groups_count;
}

#endif
