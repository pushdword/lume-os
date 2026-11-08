#ifndef LUME_MMAN_HOST_FCNTL_H
#define LUME_MMAN_HOST_FCNTL_H
#include_next <fcntl.h>
#ifndef O_CLOEXEC
#define O_CLOEXEC 0x80000
#endif
#endif
