#ifndef LUME_FILES_CLIENT_H
#define LUME_FILES_CLIENT_H
#include "lume/files.h"
#include "errno_map.h"
#include <errno.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C"
{
#endif
    int64_t lume_native_files_call(unsigned, struct lume_files_request *);
#ifdef LUME_POSIX_STAT64
#define lume_native_stat_at lume_native_stat64_at
#else
int32_t lume_native_stat_at(int64_t, const char *, unsigned, struct stat *);
#endif
#if defined(_PICOLIBC__) && __LARGEFILE64_VISIBLE
    int32_t lume_native_stat64_at(int64_t, const char *, unsigned, struct stat64 *);
#endif
#ifdef __cplusplus
}
#endif

static inline int lume_files_result(int64_t result)
{
    if (result >= 0)
        return (int)result;
    errno = lume_errno_from_syscall((unsigned)-result);
    return -1;
}
#endif
