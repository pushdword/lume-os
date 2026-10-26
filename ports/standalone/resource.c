#include "native.h"
#include <sys/resource.h>
#include <lume/credential.h>

static int resource(int which, struct rlimit *limit, int set)
{
    unsigned index;
    switch (which)
    {
    case RLIMIT_NOFILE:
        index = LUME_LIMIT_FILES;
        break;
    case RLIMIT_NPROC:
        index = LUME_LIMIT_PROCESSES;
        break;
    case RLIMIT_FSIZE:
        index = LUME_LIMIT_FILE_SIZE;
        break;
    case RLIMIT_CORE:
        index = LUME_LIMIT_CORE_SIZE;
        break;
    default:
        errno = ENOTSUP;
        return -1;
    }
    struct lume_limit_request request = {.version = LUME_CREDENTIAL_VERSION,
                                         .bytes = sizeof(request),
                                         .resource = index,
                                         .soft = limit->rlim_cur,
                                         .hard = limit->rlim_max};
    int result = native_result(LUME_SYS_CREDENTIAL,
                               set ? LUME_CREDENTIAL_LIMIT_SET : LUME_CREDENTIAL_LIMIT_GET,
                               (uintptr_t)&request, sizeof(request));
    if (!result)
    {
        limit->rlim_cur = request.soft;
        limit->rlim_max = request.hard;
    }
    return result;
}

int getrlimit(int which, struct rlimit *limit)
{
    *limit = (struct rlimit){0, 0};
    return resource(which, limit, 0);
}

int setrlimit(int which, const struct rlimit *limit)
{
    struct rlimit copy = *limit;
    return resource(which, &copy, 1);
}
