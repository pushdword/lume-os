#ifndef LUME_OPENSSH_UTIL_H
#define LUME_OPENSSH_UTIL_H
#include <termios.h>
struct winsize;
int openpty(int *, int *, char *, const struct termios *, const struct winsize *);
int login_tty(int);
#endif
