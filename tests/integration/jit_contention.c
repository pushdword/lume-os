/* SPDX-License-Identifier: BSD-3-Clause */
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

static unsigned ready, start;
static unsigned failures, first_error, completed;

static void fail(unsigned error)
{
    unsigned empty = 0;
    __atomic_compare_exchange_n(&first_error, &empty, error, false, __ATOMIC_RELAXED,
                                __ATOMIC_RELAXED);
    __atomic_fetch_add(&failures, 1, __ATOMIC_RELAXED);
}

static void *worker(void *argument)
{
    unsigned slot = (uintptr_t)argument;
    size_t bytes = slot < 4 ? 4096 : 2 * 1024 * 1024;
    int protection = PROT_READ | PROT_WRITE | (slot < 4 ? PROT_EXEC : 0);
    int flags = MAP_PRIVATE | MAP_ANONYMOUS | (slot < 4 ? MAP_JIT : 0);
    volatile unsigned char *mapping = mmap(NULL, bytes, protection, flags, -1, 0);
    if (mapping == MAP_FAILED)
        fail((unsigned)errno);
    __atomic_fetch_add(&ready, 1, __ATOMIC_RELEASE);
    while (!__atomic_load_n(&start, __ATOMIC_ACQUIRE))
        usleep(1000);
    if (mapping == MAP_FAILED)
        return NULL;
    if (slot < 4)
    {
        for (unsigned i = 0; i < 10000; ++i)
        {
            if (lume_jit_write_begin())
            {
                fail((unsigned)errno);
                break;
            }
            if (lume_jit_write_begin())
            {
                fail((unsigned)errno);
                (void)lume_jit_write_end();
                break;
            }
            mapping[0] = (unsigned char)(i + slot);
            int inner = lume_jit_write_end(), inner_error = errno;
            int outer = lume_jit_write_end(), outer_error = errno;
            if (inner || outer)
            {
                fail((unsigned)(inner ? inner_error : outer_error));
                break;
            }
            if (mapping[0] != (unsigned char)(i + slot))
            {
                fail(1000);
                break;
            }
            __atomic_fetch_add(&completed, 1, __ATOMIC_RELAXED);
        }
    }
    else
    {
        for (unsigned i = 0; i < 64; ++i)
        {
            for (size_t at = 0; at < bytes; at += 4096)
                mapping[at] = (unsigned char)i;
            if (madvise((void *)mapping, bytes, MADV_DONTNEED))
            {
                fail((unsigned)errno);
                break;
            }
        }
    }
    if (munmap((void *)mapping, bytes))
        fail((unsigned)errno);
    return NULL;
}

static void *fault(void *unused)
{
    (void)unused;
#if defined(__aarch64__)
    __asm__ volatile("brk #0" ::: "memory");
#elif defined(__i386__) || defined(__x86_64__)
    __asm__ volatile("ud2" ::: "memory");
#endif
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "--fault-child"))
    {
        pid_t child = fork();
        if (child < 0)
            return 1;
        if (!child)
        {
            pthread_t thread;
            if (pthread_create(&thread, NULL, fault, NULL))
                _exit(1);
            (void)pthread_join(thread, NULL);
            _exit(2);
        }
        int status = 0;
        if (waitpid(child, &status, 0) != child)
            return 2;
        printf("JIT_FAULT_CHILD status=%d responsive=1\n", status);
        return WIFSIGNALED(status) || (WIFEXITED(status) && WEXITSTATUS(status) == 139) ? 0 : 3;
    }
    pthread_t threads[8];
    for (unsigned i = 0; i < 8; ++i)
        if (pthread_create(&threads[i], NULL, worker, (void *)(uintptr_t)i))
            _exit(1);
    while (__atomic_load_n(&ready, __ATOMIC_ACQUIRE) != 8)
        usleep(1000);
    __atomic_store_n(&start, 1, __ATOMIC_RELEASE);
    for (unsigned i = 0; i < 8; ++i)
        if (pthread_join(threads[i], NULL))
            fail((unsigned)errno);
    printf("JIT_CONTENTION scopes=%u failures=%u errno=%u\n", completed, failures, first_error);
    return failures || completed != 40000;
}
