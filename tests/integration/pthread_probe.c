#define _POSIX_C_SOURCE 200809L
#define _POSIX_MONOTONIC_CLOCK 200809L
#include "api.h"
#include "pthread.h"
#include "perf.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
static pthread_once_t once = PTHREAD_ONCE_INIT;
static pthread_key_t key;
static unsigned ready, go, sum, once_count, destroyed, masks, failed;
static unsigned stop_load, detached_done;
static pthread_t initial_thread;
static int shared_fd;
static _Thread_local unsigned private_value = 17;

static void initialize(void)
{
    ++once_count;
}

static void destroy(void *value)
{
    if (value)
        __atomic_add_fetch(&destroyed, 1, __ATOMIC_RELAXED);
}

static void *run(void *argument)
{
    unsigned id = (unsigned)(uintptr_t)argument;
    if (private_value != 17 || pthread_setspecific(key, argument))
        __atomic_store_n(&failed, 1, __ATOMIC_RELAXED);
    private_value = id;
    errno = (int)(200 + id);
    pthread_once(&once, initialize);
    pthread_mutex_lock(&mutex);
    ++ready;
    pthread_cond_broadcast(&condition);
    while (!go)
        pthread_cond_wait(&condition, &mutex);
    pthread_mutex_unlock(&mutex);
    for (unsigned n = 0; n < 3000; ++n)
    {
        pthread_mutex_lock(&mutex);
        ++sum;
        pthread_mutex_unlock(&mutex);
        if (n % 16 == 0)
        {
            unsigned cpu = (unsigned)lume_syscall(LUME_INFO, LUME_CORE, 0, 0);
            __atomic_fetch_or(&masks, 1u << cpu, __ATOMIC_RELAXED);
        }
        if (private_value != id || errno != (int)(200 + id) || pthread_getspecific(key) != argument)
            __atomic_store_n(&failed, 1, __ATOMIC_RELAXED);
        unsigned char *memory = malloc(37 + id);
        if (!memory)
            __atomic_store_n(&failed, 1, __ATOMIC_RELAXED);
        else
        {
            memset(memory, (int)id, 37 + id);
            if (memory[36 + id] != id)
                __atomic_store_n(&failed, 1, __ATOMIC_RELAXED);
            free(memory);
        }
    }
    return (void *)(uintptr_t)(id + 100);
}

static void *forever(void *argument)
{
    (void)argument;
    for (;;)
        __atomic_fetch_add(&sum, 1, __ATOMIC_RELAXED);
}

static void *load(void *argument)
{
    (void)argument;
    uint64_t count = 0;
    while (!__atomic_load_n(&stop_load, __ATOMIC_RELAXED))
        ++count;
    return (void *)(uintptr_t)(count != 0);
}

static void *finish(void *argument)
{
    __atomic_add_fetch(&detached_done, 1, __ATOMIC_RELEASE);
    return argument;
}

static void *terminate(void *argument)
{
    if ((uintptr_t)argument == 1)
        app_exit(7);
    if ((uintptr_t)argument == 2)
    {
        extern unsigned char __tls_size[], __tls_align[];
        uintptr_t align = (uintptr_t)__tls_align;
        uintptr_t bias = (16 + align - 1) & ~(align - 1);
        uintptr_t pages = (bias + (uintptr_t)__tls_size + 4095) / 4096;
        *(volatile unsigned *)((uintptr_t)__builtin_thread_pointer() + pages * 4096) = 1;
    }
    *(volatile unsigned *)(uintptr_t)0x200000 = 1;
    return NULL;
}

static void *last_thread(void *argument)
{
    (void)argument;
    lume_syscall(LUME_SLEEP, 50, 0, 0);
    puts("PTHREAD_LAST_THREAD_PASS");
    return NULL;
}

static void *join_initial(void *argument)
{
    (void)argument;
    void *result;
    if (pthread_join(initial_thread, &result) || result != (void *)42)
        return (void *)1;
    puts("PTHREAD_JOIN_INITIAL_PASS");
    return NULL;
}

struct future_wait
{
    pthread_cond_t condition;
    unsigned ready, done;
};

static void *future_condition(void *argument)
{
    struct future_wait *wait = argument;
    pthread_mutex_lock(&mutex);
    wait->ready = 1;
    pthread_cond_broadcast(&wait->condition);
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    end.tv_sec += 70;
    while (!wait->done)
        if (pthread_cond_timedwait(&wait->condition, &mutex, &end))
        {
            pthread_mutex_unlock(&mutex);
            return (void *)1;
        }
    pthread_mutex_unlock(&mutex);
    return NULL;
}

static void *shared_files(void *argument)
{
    (void)argument;
    const char data[] = "thread-fd";
    if (app_write(shared_fd, data, sizeof(data)) != sizeof(data))
        return (void *)1;
    const char *argv[] = {"/bin/true", NULL};
    int pid = app_spawn("/bin/true", 1, argv);
    if (pid < 1)
        return (void *)2;
    struct lume_wait_status status;
    if (lume_syscall(LUME_WAITPID, (unsigned)pid, (uintptr_t)&status, 0) != pid || status.code)
        return (void *)3;
    return NULL;
}

static void *ramblock(void *argument)
{
    unsigned id = (unsigned)(uintptr_t)argument;
    int fd = app_open("/dev/ramblk0", LUME_RDWR);
    if (fd < 0)
        return (void *)1;
    unsigned char input[512], output[512];
    for (unsigned n = 0; n < 16; ++n)
    {
        memset(input, (int)(id + n), sizeof(input));
        if (app_seek(fd, (int32_t)((id - 1) * 512), LUME_SEEK_SET) < 0 ||
            app_write(fd, input, sizeof(input)) != sizeof(input) ||
            app_seek(fd, (int32_t)((id - 1) * 512), LUME_SEEK_SET) < 0 ||
            app_read(fd, output, sizeof(output)) != sizeof(output) ||
            memcmp(input, output, sizeof(input)))
        {
            app_close(fd);
            return (void *)2;
        }
    }
    app_close(fd);
    return NULL;
}

static int lifecycle(void)
{
    if (pthread_join(pthread_self(), NULL) != EDEADLK)
        return 10;
    for (unsigned i = 0; i < 24; ++i)
    {
        pthread_t thread;
        void *result;
        if (pthread_create(&thread, NULL, finish, (void *)(uintptr_t)(i + 1)) ||
            pthread_join(thread, &result) || (uintptr_t)result != i + 1 ||
            pthread_join(thread, NULL) != ESRCH)
            return 11;
    }
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    for (unsigned i = 0; i < 24; ++i)
    {
        pthread_t thread;
        unsigned before = __atomic_load_n(&detached_done, __ATOMIC_ACQUIRE);
        if (pthread_create(&thread, &attr, finish, NULL))
            return 12;
        while (__atomic_load_n(&detached_done, __ATOMIC_ACQUIRE) == before)
            lume_syscall(LUME_SLEEP, 1, 0, 0);
        lume_syscall(LUME_SLEEP, 1, 0, 0);
    }
    pthread_condattr_t condition_attr;
    pthread_condattr_init(&condition_attr);
    pthread_condattr_setclock(&condition_attr, CLOCK_MONOTONIC);
    pthread_cond_t timed;
    pthread_cond_init(&timed, &condition_attr);
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    end.tv_nsec += 30000000;
    if (end.tv_nsec >= 1000000000)
    {
        ++end.tv_sec;
        end.tv_nsec -= 1000000000;
    }
    pthread_mutex_lock(&mutex);
    if (pthread_cond_timedwait(&timed, &mutex, &end) != ETIMEDOUT ||
        pthread_mutex_trylock(&mutex) != EBUSY || pthread_mutex_unlock(&mutex) ||
        pthread_cond_destroy(&timed))
        return 13;
    shared_fd = app_open("/tmp/pthread-fd", LUME_RDWR | LUME_CREAT | LUME_TRUNC);
    pthread_t thread;
    void *result;
    if (shared_fd < 0 || pthread_create(&thread, NULL, shared_files, NULL) ||
        pthread_join(thread, &result) || result || app_seek(shared_fd, 0, LUME_SEEK_CUR) != 10)
        return 14;
    app_close(shared_fd);
    struct future_wait future = {0};
    pthread_cond_init(&future.condition, &condition_attr);
    if (pthread_create(&thread, NULL, future_condition, &future))
        return 15;
    pthread_mutex_lock(&mutex);
    while (!future.ready)
        pthread_cond_wait(&future.condition, &mutex);
    pthread_mutex_unlock(&mutex);
    lume_syscall(LUME_SLEEP, 20, 0, 0);
    pthread_mutex_lock(&mutex);
    future.done = 1;
    pthread_cond_signal(&future.condition);
    pthread_mutex_unlock(&mutex);
    if (pthread_join(thread, &result) || result || pthread_cond_destroy(&future.condition))
        return 16;
    puts("PTHREAD_LIFECYCLE_TIMEOUT_SHARED_FD_SPAWN_PASS");
    return 0;
}

int app_main(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "ramblock"))
    {
        pthread_t threads[4];
        for (unsigned i = 0; i < 4; ++i)
            if (pthread_create(&threads[i], NULL, ramblock, (void *)(uintptr_t)(i + 1)))
                return 40;
        for (unsigned i = 0; i < 4; ++i)
        {
            void *result;
            if (pthread_join(threads[i], &result) || result)
                return 41;
        }
        puts("PTHREAD_RAMBLOCK_PASS");
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "joinmain"))
    {
        initial_thread = pthread_self();
        pthread_t thread;
        if (pthread_create(&thread, NULL, join_initial, NULL))
            return 1;
        pthread_exit((void *)42);
    }
    if (argc == 2 && !strcmp(argv[1], "idle"))
    {
        struct lume_perf_snapshot before, after;
        puts("IDLE_MEASURE_BEGIN");
        lume_syscall(LUME_SLEEP, 100, 0, 0);
        if (lume_syscall(LUME_PERF, (uintptr_t)&before, sizeof(before), LUME_PERF_VERSION))
            return 30;
        lume_syscall(LUME_SLEEP, 5000, 0, 0);
        if (lume_syscall(LUME_PERF, (uintptr_t)&after, sizeof(after), LUME_PERF_VERSION))
            return 31;
        for (unsigned c = 0; c < after.core_count; ++c)
        {
            uint32_t busy =
                (after.cores[c].ticks[LUME_PERF_USER] - before.cores[c].ticks[LUME_PERF_USER]) +
                (after.cores[c].ticks[LUME_PERF_KERNEL] - before.cores[c].ticks[LUME_PERF_KERNEL]) +
                (after.cores[c].ticks[LUME_PERF_LOCKWAIT] -
                 before.cores[c].ticks[LUME_PERF_LOCKWAIT]);
            printf("IDLE_CPU=%u BUSY=%u WALL=%u\n", c, busy,
                   after.cores[c].counter - before.cores[c].counter);
        }
        for (unsigned i = 0; i < before.process_count; ++i)
            for (unsigned j = 0; j < after.process_count; ++j)
                if (before.processes[i].pid == 1 && after.processes[j].pid == 1)
                    printf("IDLE_SHELL_TICKS=%u\n",
                           after.processes[j].user_ticks - before.processes[i].user_ticks +
                               after.processes[j].kernel_ticks - before.processes[i].kernel_ticks);
        puts("IDLE_MEASURE_PASS");
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "monitor"))
    {
        pthread_t threads[4];
        for (unsigned i = 0; i < 4; ++i)
            if (pthread_create(&threads[i], NULL, load, NULL))
                return 20;
        const char *args[] = {"/usr/bin/htop", NULL};
        int pid = app_spawn(args[0], 1, args);
        struct lume_wait_status status;
        if (pid < 1 || lume_syscall(LUME_WAITPID, (unsigned)pid, (uintptr_t)&status, 0) != pid ||
            status.code)
            return 21;
        __atomic_store_n(&stop_load, 1, __ATOMIC_RELAXED);
        for (unsigned i = 0; i < 4; ++i)
        {
            void *result;
            if (pthread_join(threads[i], &result) || result != (void *)1)
                return 22;
        }
        puts("PTHREAD_HTOP_LOAD_PASS");
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "mainexit"))
    {
        pthread_t thread;
        if (pthread_create(&thread, NULL, last_thread, NULL))
            return 1;
        pthread_exit(NULL);
    }
    if (argc == 2 &&
        (!strcmp(argv[1], "threadexit") || !strcmp(argv[1], "fault") || !strcmp(argv[1], "guard")))
    {
        pthread_t thread;
        void *mode = (void *)(uintptr_t)(!strcmp(argv[1], "threadexit") ? 1
                                         : !strcmp(argv[1], "guard")    ? 2
                                                                        : 0);
        if (pthread_create(&thread, NULL, terminate, mode))
            return 1;
        for (;;)
            lume_syscall(LUME_SLEEP, 10, 0, 0);
    }
    if (argc == 2 && !strcmp(argv[1], "exit"))
    {
        pthread_t threads[2];
        if (pthread_create(&threads[0], NULL, forever, NULL) ||
            pthread_create(&threads[1], NULL, forever, NULL))
            return 1;
        lume_syscall(LUME_SLEEP, 300, 0, 0);
        return 0;
    }
    unsigned cpus = (unsigned)lume_syscall(LUME_INFO, LUME_CORES, 0, 0);
    printf("PTHREAD_PROBE_CPUS=%u\n", cpus);
    if (cpus != 1 && cpus != 4)
        return 2;
    if (pthread_key_create(&key, destroy))
        return 3;
    private_value = 99;
    pthread_t threads[4];
    for (unsigned i = 0; i < 4; ++i)
        if (pthread_create(&threads[i], NULL, run, (void *)(uintptr_t)(i + 1)))
            return 4;
    pthread_mutex_lock(&mutex);
    while (ready != 4)
        pthread_cond_wait(&condition, &mutex);
    go = 1;
    pthread_cond_broadcast(&condition);
    pthread_mutex_unlock(&mutex);
    for (unsigned i = 0; i < 4; ++i)
    {
        void *value;
        if (pthread_join(threads[i], &value) || (uintptr_t)value != i + 101)
            return 5;
    }
    if (sum != 12000 || once_count != 1 || destroyed != 4 || private_value != 99 || failed)
    {
        printf("PTHREAD_FAILURE sum=%u once=%u destructors=%u tls=%u failed=%u\n", sum, once_count,
               destroyed, private_value, failed);
        return 6;
    }
    if (masks != (1u << cpus) - 1)
    {
        printf("PTHREAD_CPU_MASK=%u\n", masks);
        return 7;
    }
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_t recursive;
    pthread_mutex_init(&recursive, &attr);
    if (pthread_mutex_lock(&recursive) || pthread_mutex_lock(&recursive) ||
        pthread_mutex_unlock(&recursive) || pthread_mutex_unlock(&recursive))
        return 8;
    if (pthread_mutex_destroy(&recursive) || pthread_key_delete(key))
        return 9;
    printf("PTHREAD_SMP_TLS_MUTEX_COND_ONCE_KEYS_PASS mask=%u\n", masks);
    return lifecycle();
}
