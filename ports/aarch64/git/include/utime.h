#ifndef LUME_GIT_UTIME_H
#define LUME_GIT_UTIME_H
#include <sys/utime.h>
int utime(const char *, const struct utimbuf *);
#endif
