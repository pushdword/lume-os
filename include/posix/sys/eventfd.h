/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_POSIX_EVENTFD_H
#define LUME_POSIX_EVENTFD_H
#include <stdint.h>
#include <fcntl.h>
#define EFD_CLOEXEC O_CLOEXEC
#define EFD_NONBLOCK O_NONBLOCK
#define EFD_SEMAPHORE 1
typedef uint64_t eventfd_t;
#ifdef __cplusplus
extern "C"
{
#endif
    int eventfd(unsigned, int);
    int eventfd_read(int, eventfd_t *);
    int eventfd_write(int, eventfd_t);
#ifdef __cplusplus
}
#endif
#endif
