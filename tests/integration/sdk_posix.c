#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

static _Thread_local unsigned local_value = 7;
static unsigned failed;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void require(int valid, const char *operation)
{
    if (!valid)
    {
        fprintf(stderr, "SDK_POSIX_FAIL %s errno=%d\n", operation, errno);
        exit(1);
    }
}

static void *worker(void *argument)
{
    unsigned count = (unsigned)(uintptr_t)argument;
    unsigned error = local_value != 7;
    local_value = count + 10;
    pid_t pid = getpid();
    struct pollfd record = {.fd = -1, .events = POLLIN};
    for (unsigned i = 0; i < count; ++i)
        error |= poll(&record, 1, 0) != 0 || record.revents != 0 || getpid() != pid;
    error |= local_value != count + 10;
    pthread_mutex_lock(&mutex);
    failed |= error;
    pthread_mutex_unlock(&mutex);
    return NULL;
}

static uint64_t nanoseconds(void)
{
    struct timespec now;
    require(!clock_gettime(CLOCK_MONOTONIC, &now), "clock_gettime");
    return (uint64_t)now.tv_sec * UINT64_C(1000000000) + now.tv_nsec;
}

int main(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "--bench"))
    {
        for (unsigned threads = 1; threads <= 4; threads *= 4)
        {
            pthread_t handles[4];
            uint64_t begin = nanoseconds();
            for (unsigned i = 0; i < threads; ++i)
                require(!pthread_create(handles + i, NULL, worker, (void *)(uintptr_t)50000),
                        "pthread_create");
            for (unsigned i = 0; i < threads; ++i)
                require(!pthread_join(handles[i], NULL), "pthread_join");
            uint64_t elapsed = nanoseconds() - begin;
            require(!failed && local_value == 7, "thread isolation");
            printf("SDK_POSIX_BENCH threads=%u polls=%u elapsed_ns=%llu\n", threads,
                   threads * 50000, (unsigned long long)elapsed);
        }
        return 0;
    }
    char output[32];
    require(snprintf(output, sizeof(output), "%s:%d", "posix", 42) == 8 &&
                !strcmp(output, "posix:42"),
            "stdio");
    int descriptors[2];
    require(!pipe(descriptors), "pipe");
    struct pollfd records[2] = {{.fd = descriptors[0], .events = POLLIN},
                                {.fd = -1, .events = POLLIN, .revents = POLLIN}};
    require(!poll(records, 2, 0) && !records[0].revents && !records[1].revents, "idle poll");
    struct iovec vector[2] = {{.iov_base = "abc", .iov_len = 3}, {.iov_base = "def", .iov_len = 3}};
    require(writev(descriptors[1], vector, 2) == 6, "writev");
    require(poll(records, 2, 0) == 1 && records[0].revents == POLLIN, "ready poll");
    require(read(descriptors[0], output, 6) == 6 && !memcmp(output, "abcdef", 6), "read");
    require(!close(descriptors[1]), "close writer");
    require(poll(records, 1, 0) == 1 && (records[0].revents & POLLHUP), "hangup poll");
    require(!close(descriptors[0]), "close reader");
    errno = 0;
    require(read(-1, output, 1) == -1 && errno == EBADF, "errno");
    require(!socketpair(AF_UNIX, SOCK_STREAM, 0, descriptors), "socketpair");
    require(write(descriptors[0], "ipc", 3) == 3 && read(descriptors[1], output, 3) == 3 &&
                !memcmp(output, "ipc", 3),
            "local IPC");
    require(!close(descriptors[0]) && !close(descriptors[1]), "close sockets");
    void *memory = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    require(memory != MAP_FAILED, "mmap");
    memset(memory, 0xa5, 4096);
    require(((unsigned char *)memory)[4095] == 0xa5 && !munmap(memory, 4096), "munmap");
    uint64_t before = nanoseconds();
    pthread_t thread;
    require(!pthread_create(&thread, NULL, worker, (void *)(uintptr_t)100), "pthread_create");
    require(!pthread_join(thread, NULL) && !failed && local_value == 7, "pthread_join and TLS");
    require(nanoseconds() >= before, "monotonic clock");
    puts("SDK_POSIX_PASS stdio errno pipes poll writev sockets mmap threads TLS clocks");
    return 0;
}
