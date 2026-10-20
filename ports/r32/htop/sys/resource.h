#ifndef LUME_HTOP_RESOURCE_H
#define LUME_HTOP_RESOURCE_H
#include_next <sys/resource.h>
#ifndef HTOP_HOST_TEST
#include <sys/types.h>
#define PRIO_PROCESS 0
int getpriority(int, id_t);
int setpriority(int, id_t, int);
#endif
#endif
