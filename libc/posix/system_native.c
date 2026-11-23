/* SPDX-License-Identifier: BSD-3-Clause */
#include "native_posix.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sched.h>
#include <poll.h>
#include <fcntl.h>
#include "lume/credential.h"

int sched_yield(void)
{
    return native_result(native_call(LUME_SYS_SLEEP, 0, 0, 0));
}

int sched_get_priority_min(int policy)
{
    if (policy == SCHED_OTHER)
        return 0;
    errno = EINVAL;
    return -1;
}

int sched_get_priority_max(int policy)
{
    return sched_get_priority_min(policy);
}

mode_t umask(mode_t mask)
{
    struct lume_credential_request r = {
        .version = LUME_CREDENTIAL_VERSION, .bytes = sizeof(r), .uid = mask};
    return native_result(
        native_call(LUME_SYS_CREDENTIAL, LUME_CREDENTIAL_UMASK, (uintptr_t)&r, sizeof(r)));
}

unsigned sleep(unsigned seconds)
{
    while (seconds)
    {
        unsigned step = seconds > 60 ? 60 : seconds;
        if (poll(NULL, 0, (int)step * 1000) < 0)
            return seconds;
        seconds -= step;
    }
    return 0;
}

int link(const char *source, const char *target)
{
    return linkat(AT_FDCWD, source, AT_FDCWD, target, 0);
}

int getentropy(void *buffer, size_t bytes)
{
    if (bytes > 256)
    {
        errno = EIO;
        return -1;
    }
    return native_result(native_call(LUME_SYS_RANDOM, (uintptr_t)buffer, bytes, 0)) < 0 ? -1 : 0;
}

int gethostname(char *name, size_t bytes)
{
    if (bytes < sizeof("lume"))
    {
        errno = ENAMETOOLONG;
        return -1;
    }
    memcpy(name, "lume", sizeof("lume"));
    return 0;
}

int uname(struct utsname *out)
{
    memset(out, 0, sizeof(*out));
    strcpy(out->sysname, "Lume");
    strcpy(out->nodename, "lume");
    strcpy(out->release, "1.0");
    strcpy(out->version, "Native ABI 1.0");
#if defined(LUME_ARCH_AMD64)
    strcpy(out->machine, "x86_64");
#elif defined(LUME_ARCH_AARCH64)
    strcpy(out->machine, "aarch64");
#elif defined(LUME_ARCH_I686)
    strcpy(out->machine, "i686");
#else
    strcpy(out->machine, "x32");
#endif
    return 0;
}
