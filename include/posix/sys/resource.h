#ifndef LUME_PORT_RESOURCE_H
#define LUME_PORT_RESOURCE_H
#include <sys/time.h>
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
#define PRIO_PROCESS 0
#define PRIO_PGRP 1
#define PRIO_USER 2
#define RUSAGE_SELF 0
#define RUSAGE_CHILDREN -1
#define RUSAGE_THREAD 1

struct rusage
{
    struct timeval ru_utime;
    struct timeval ru_stime;
    long ru_maxrss;
    long ru_ixrss;
    long ru_idrss;
    long ru_isrss;
    long ru_minflt;
    long ru_majflt;
    long ru_nswap;
    long ru_inblock;
    long ru_oublock;
    long ru_msgsnd;
    long ru_msgrcv;
    long ru_nsignals;
    long ru_nvcsw;
    long ru_nivcsw;
};

#ifdef __cplusplus
extern "C"
{
#endif
    int getrlimit(int, struct rlimit *);
    int setrlimit(int, const struct rlimit *);
    int getpriority(int, id_t);
    int setpriority(int, id_t, int);
    int getrusage(int, struct rusage *);
#ifdef __cplusplus
}
#endif
#endif
