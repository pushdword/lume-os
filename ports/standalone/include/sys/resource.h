#ifndef LUME_PORT_RESOURCE_H
#define LUME_PORT_RESOURCE_H
#include <sys/types.h>
typedef unsigned long rlim_t;

struct rlimit
{
    rlim_t rlim_cur, rlim_max;
};

#define RLIMIT_FSIZE 1
#define RLIMIT_DATA 2
#define RLIMIT_STACK 3
#define RLIMIT_CORE 4
#define RLIMIT_NPROC 6
#define RLIMIT_NOFILE 7
#define RLIMIT_AS 9
#define RLIM_INFINITY (~0ul)
#ifdef __cplusplus
extern "C"
{
#endif
    int getrlimit(int, struct rlimit *);
    int setrlimit(int, const struct rlimit *);
#ifdef __cplusplus
}
#endif
#endif
