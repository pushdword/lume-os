#ifndef LUME_POSIX_PTY_H
#define LUME_POSIX_PTY_H
#include <termios.h>
#include <sys/ioctl.h>
#ifdef __cplusplus
extern "C"
{
#endif
    int posix_openpt(int);
    int grantpt(int);
    int unlockpt(int);
    char *ptsname(int);
    int openpty(int *, int *, char *, const struct termios *, const struct winsize *);
#ifdef __cplusplus
}
#endif
#endif
