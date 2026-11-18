#include "lume/abi.h"
#include "lume/credential.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint16_t uid_t;
typedef uint16_t gid_t;
static struct lume_credential_request captured;
static unsigned calls, expected, checks;
static int64_t result;
static uint32_t group_count = 2;

#define T(x)                                                                                       \
    do                                                                                             \
    {                                                                                              \
        assert(x);                                                                                 \
        ++checks;                                                                                  \
    } while (0)

static int64_t native_call(unsigned number, unsigned operation, uintptr_t address, size_t bytes)
{
    assert(number == LUME_SYS_CREDENTIAL && operation == expected && bytes == sizeof(captured));
    struct lume_credential_request *r = (void *)address;
    assert(r->version == LUME_CREDENTIAL_VERSION && r->bytes == sizeof(*r) && !r->reserved);
    captured = *r;
    ++calls;
    if (!result && operation == LUME_CREDENTIAL_GET)
    {
        r->groups_count = group_count;
        r->groups[0] = 20;
        r->groups[1] = 30;
    }
    return result;
}

static int native_result(int64_t value)
{
    if (value >= 0)
        return (int)value;
    errno = value == -LUME_EPERM ? EPERM : EINVAL;
    return -1;
}

#include "../../libc/posix/credential_calls.h"

int main(void)
{
    expected = LUME_CREDENTIAL_SETUID;
    T(setuid(1000) == 0 && captured.uid == 1000 && captured.suid == UINT32_MAX);
    T(setuid((uid_t)-1) == 0 && captured.uid == UINT16_MAX);
    expected = LUME_CREDENTIAL_SETGID;
    T(setgid(20) == 0 && captured.gid == 20 && captured.sgid == UINT32_MAX);
    expected = LUME_CREDENTIAL_SETEUID;
    T(seteuid(1000) == 0 && captured.uid == 1000);
    expected = LUME_CREDENTIAL_SETEGID;
    T(setegid(20) == 0 && captured.gid == 20);
    expected = LUME_CREDENTIAL_SETREUID;
    T(setreuid(-1, 1000) == 0 && captured.uid == UINT32_MAX && captured.euid == 1000);
    T(setreuid(1000, -1) == 0 && captured.uid == 1000 && captured.euid == UINT32_MAX);
    expected = LUME_CREDENTIAL_SETREGID;
    T(setregid(-1, 20) == 0 && captured.gid == UINT32_MAX && captured.egid == 20);
    expected = LUME_CREDENTIAL_UID;
    T(setresuid(-1, 1000, 2000) == 0 && captured.uid == UINT32_MAX && captured.euid == 1000 &&
      captured.suid == 2000);
    expected = LUME_CREDENTIAL_GID;
    T(setresgid(20, -1, 30) == 0 && captured.gid == 20 && captured.egid == UINT32_MAX &&
      captured.sgid == 30);
    result = -LUME_EPERM;
    expected = LUME_CREDENTIAL_SETUID;
    T(setuid(0) == -1 && errno == EPERM);
    result = 0;
    unsigned before = calls;
    T(setgroups(-1, NULL) == -1 && errno == EINVAL);
    T(setgroups(LUME_GROUP_MAX + 1, NULL) == -1 && errno == EINVAL);
    T(setgroups(1, NULL) == -1 && errno == EFAULT && calls == before);
    expected = LUME_CREDENTIAL_GROUPS;
    T(setgroups(0, NULL) == 0 && captured.groups_count == 0);
    gid_t groups[] = {20, 30};
    T(setgroups(2, groups) == 0 && captured.groups_count == 2 && captured.groups[0] == 20 &&
      captured.groups[1] == 30);
    before = calls;
    T(getgroups(-1, groups) == -1 && errno == EINVAL && calls == before);
    expected = LUME_CREDENTIAL_GET;
    T(getgroups(0, NULL) == 2);
    gid_t output[] = {99, 99};
    T(getgroups(1, output) == -1 && errno == EINVAL && output[0] == 99);
    T(getgroups(2, NULL) == -1 && errno == EFAULT);
    T(getgroups(2, output) == 2 && output[0] == 20 && output[1] == 30);
    group_count = 0;
    T(getgroups(2, NULL) == 0);
    printf("credential calls: %u checks passed\n", checks);
    return 0;
}
