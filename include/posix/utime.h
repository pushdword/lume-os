#ifndef LUME_POSIX_UTIME_H
#define LUME_POSIX_UTIME_H
#include <time.h>
#include_next <utime.h>

#ifdef __cplusplus
extern "C"
{
#endif

    int utime(const char *, const struct utimbuf *);

#ifdef __cplusplus
}
#endif
#endif
