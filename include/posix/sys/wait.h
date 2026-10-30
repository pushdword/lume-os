#ifndef LUME_POSIX_WAIT_H
#define LUME_POSIX_WAIT_H
#include_next <sys/wait.h>
#ifndef WCONTINUED
#define WCONTINUED 8
#endif
#ifndef WIFCONTINUED
#define WIFCONTINUED(status) ((status) == 0xffff)
#endif
#endif
