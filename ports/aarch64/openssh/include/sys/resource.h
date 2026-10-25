#ifndef LUME_SSH_RESOURCE_H
#define LUME_SSH_RESOURCE_H
#include <sys/types.h>
typedef unsigned long rlim_t;

struct rlimit
{
    rlim_t rlim_cur, rlim_max;
};

#define RLIMIT_CORE 4
#define RLIMIT_NOFILE 7
#define RLIM_INFINITY (~0ul)
int getrlimit(int, struct rlimit *);
int setrlimit(int, const struct rlimit *);
#define RLIMIT_FSIZE 1
#define RLIMIT_NPROC 6
#define RLIMIT_AS 9
#define RLIMIT_DATA 2
#define RLIMIT_STACK 3
#endif
