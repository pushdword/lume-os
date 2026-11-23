#ifndef LUME_POSIX_UNISTD_H
#define LUME_POSIX_UNISTD_H
#include_next <unistd.h>

#ifdef __cplusplus
extern "C"
{
#endif

    int fchdir(int);
    int linkat(int, const char *, int, const char *, int);
    int symlinkat(const char *, int, const char *);

#ifdef __cplusplus
}
#endif
#endif
