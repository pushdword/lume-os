/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_POSIX_SEMAPHORE_H
#define LUME_POSIX_SEMAPHORE_H
#include <limits.h>
#include <stdint.h>
#include <time.h>

typedef struct
{
    uint64_t counters __attribute__((aligned(8)));
    uint32_t state;
} sem_t;

#ifndef SEM_VALUE_MAX
#define SEM_VALUE_MAX INT_MAX
#endif
#define SEM_FAILED ((sem_t *)-1)

#ifdef __cplusplus
extern "C"
{
#endif
    int sem_init(sem_t *, int, unsigned);
    int sem_destroy(sem_t *);
    int sem_wait(sem_t *);
    int sem_trywait(sem_t *);
    int sem_timedwait(sem_t *, const struct timespec *);
    int sem_post(sem_t *);
    int sem_getvalue(sem_t *, int *);
    sem_t *sem_open(const char *, int, ...);
    int sem_close(sem_t *);
    int sem_unlink(const char *);
#ifdef __cplusplus
}
#endif
#endif
