#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <lume/abi.h>
#include <limits.h>
#include <semaphore.h>
#include <sys/stat.h>

#if defined(__aarch64__) || defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) ||              \
    defined(LUME_ARCH_I686)
#include "lume/syscall_native.h"
#include "lume/process.h"
#include "lume/credential.h"
#include "errno_map.h"

static int query_result(int64_t result)
{
    if (result >= 0)
        return 0;
    errno = lume_errno_from_syscall((unsigned)-result);
    return -1;
}

static long configuration_value(uint64_t value)
{
    if (value > LONG_MAX)
    {
        errno = EOVERFLOW;
        return -1;
    }
    return (long)value;
}

static int features(struct lume_system_features *value)
{
    return query_result(lume_native_call(LUME_SYS_FEATURES, (uintptr_t)value, sizeof(*value),
                                         LUME_FEATURE_VERSION));
}

static long resource_limit(unsigned resource, uint64_t capacity)
{
    struct lume_limit_request request = {
        .version = LUME_CREDENTIAL_VERSION, .bytes = sizeof(request), .resource = resource};
    if (query_result(lume_native_call(LUME_SYS_CREDENTIAL, LUME_CREDENTIAL_LIMIT_GET,
                                      (uintptr_t)&request, sizeof(request))))
        return -1;
    return configuration_value(request.soft < capacity ? request.soft : capacity);
}
#endif

long sysconf(int name)
{
    switch (name)
    {
    case _SC_ARG_MAX:
        return LUME_ARG_BYTES;
    case _SC_OPEN_MAX:
#if defined(__aarch64__) || defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) ||              \
    defined(LUME_ARCH_I686)
    {
        struct lume_system_features value;
        return features(&value) ? -1 : resource_limit(LUME_LIMIT_FILES, value.fd_limit);
    }
    case _SC_CHILD_MAX:
    case _SC_THREAD_THREADS_MAX:
    {
        struct lume_system_features value;
        if (features(&value))
            return -1;
        if (name == _SC_THREAD_THREADS_MAX && !(value.features & LUME_FEATURE_THREADS))
            return -1;
        return resource_limit(LUME_LIMIT_PROCESSES, value.task_limit);
    }
    case _SC_NPROCESSORS_CONF:
    case _SC_NPROCESSORS_ONLN:
    {
        int64_t value = lume_native_call(LUME_SYS_CPU, 0, 0, 0);
        return query_result(value) ? -1 : configuration_value((uint64_t)value);
    }
    case _SC_PHYS_PAGES:
    case _SC_AVPHYS_PAGES:
    {
        struct lume_memory_snapshot value;
        if (query_result(lume_native_call(LUME_SYS_MEMORY, (uintptr_t)&value, sizeof(value), 0)))
            return -1;
        return configuration_value(name == _SC_PHYS_PAGES ? value.total_pages : value.free_pages);
    }
#else
        return LUME_FD_MAX;
#endif
    case _SC_PAGESIZE:
        return 4096;
    case _SC_SEM_VALUE_MAX:
        return SEM_VALUE_MAX;
    case _SC_SEM_NSEMS_MAX:
    case _SC_SEMAPHORES:
    case _SC_CPUTIME:
    case _SC_THREAD_CPUTIME:
    case _SC_PRIORITY_SCHEDULING:
    case _SC_THREAD_PRIORITY_SCHEDULING:
        return -1;
    default:
        errno = EINVAL;
        return -1;
    }
}

clock_t clock(void)
{
    errno = ENOSYS;
    return (clock_t)-1;
}

int getloadavg(double *samples, int count)
{
    (void)samples;
    (void)count;
    errno = ENOSYS;
    return -1;
}

static long pathname_limit(int name)
{
    switch (name)
    {
    case _PC_NAME_MAX:
        return NAME_MAX;
    case _PC_PATH_MAX:
        return PATH_MAX;
    case _PC_NO_TRUNC:
        return 1;
    default:
        errno = EINVAL;
        return -1;
    }
}

long pathconf(const char *path, int name)
{
    struct stat metadata;
    return stat(path, &metadata) ? -1 : pathname_limit(name);
}

long fpathconf(int fd, int name)
{
    struct stat metadata;
    return fstat(fd, &metadata) ? -1 : pathname_limit(name);
}
