#include "lume/abi.h"
#include "lume/process.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static volatile unsigned shared_value = 17;
static volatile unsigned waiting, done;

static int64_t native(unsigned number, uint64_t a, uint64_t b, uint64_t c)
{
    register uint64_t x8 __asm__("x8") = number;
    register uint64_t x0 __asm__("x0") = a, x1 __asm__("x1") = b, x2 __asm__("x2") = c;
    __asm__ volatile("svc #0x4c55" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2) : "memory", "cc");
    return (int64_t)x0;
}

static void require(int value, const char *name)
{
    if (!value)
    {
        fprintf(stderr, "PROCESS_PROBE_FAIL %s\n", name);
        exit(1);
    }
}

static void *thread(void *unused)
{
    (void)unused;
    __atomic_store_n(&waiting, 1, __ATOMIC_RELEASE);
    while (!__atomic_load_n(&done, __ATOMIC_ACQUIRE))
        (void)native(LUME_SYS_SLEEP, 1, 0, 0);
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc == 5 && !strcmp(argv[1], "exec"))
    {
        require((unsigned)getpid() == (unsigned)atoi(argv[2]), "exec PID");
        require(getenv("EXEC_PROBE") && !strcmp(getenv("EXEC_PROBE"), "retained"),
                "exec environment");
        char bytes[2];
        require(read(atoi(argv[3]), bytes, 2) == 2 && !memcmp(bytes, "ef", 2), "exec descriptor");
        errno = 0;
        require(read(atoi(argv[4]), bytes, 1) == -1 && errno == EBADF, "exec close-on-exec");
        require(shared_value == 17, "exec image reset");
        puts("PROCESS_EXEC_PASS");
        return 23;
    }
    require(argc == 1, "arguments");
    unsigned char random[256], other[256];
    require(native(LUME_SYS_RANDOM, (uintptr_t)random, sizeof(random), 0) == sizeof(random),
            "entropy read");
    require(native(LUME_SYS_RANDOM, (uintptr_t)other, sizeof(other), 0) == sizeof(other),
            "entropy second read");
    require(memcmp(random, other, sizeof(random)) != 0, "entropy freshness");
    require(native(LUME_SYS_RANDOM, (uintptr_t)random, 257, 0) == -LUME_EINVAL,
            "entropy size bound");
    require(native(LUME_SYS_RANDOM, 0, 1, 0) == -LUME_EFAULT, "entropy buffer validation");
    unsigned char *heap = malloc(200000);
    require(heap != NULL, "heap allocation");
    for (unsigned i = 0; i < 200000; ++i)
        heap[i] = (unsigned char)(i * 13);
    int fd = open("/data/process-probe.tmp", O_RDWR | O_CREAT | O_TRUNC, 0600);
    int closed = open("/data/process-probe.tmp", O_RDONLY);
    require(fd >= 0 && closed >= 0 && write(fd, "abcdef", 6) == 6 && !lseek(fd, 0, SEEK_SET),
            "file setup");
    require(!fcntl(closed, F_SETFD, FD_CLOEXEC), "descriptor flag");
    for (unsigned round = 0; round < 3; ++round)
    {
        volatile unsigned stack_value = 29;
        int64_t pid = native(LUME_SYS_FORK, 0, 0, 0);
        require(pid >= 0, "fork result");
        if (!pid)
        {
            require(shared_value == 17 && stack_value == 29, "fork data and stack");
            for (unsigned i = 0; i < 200000; ++i)
                require(heap[i] == (unsigned char)(i * 13), "fork heap contents");
            shared_value = 31;
            stack_value = 37;
            memset(heap, 0, 200000);
            char bytes[2];
            require(read(fd, bytes, 2) == 2 && !memcmp(bytes, "ab", 2), "fork shared offset");
            char *bad_args[] = {argv[0], NULL};
            struct lume_run_request bad = {.path = (uintptr_t)"/data/absent-executable",
                                           .argv = (uintptr_t)bad_args,
                                           .argc = 1};
            require(native(LUME_SYS_EXEC, (uintptr_t)&bad, 0, 0) == -LUME_ENOENT,
                    "failed exec preserves image");
            require(shared_value == 31 && heap[100000] == 0, "failed exec memory");
            char pid_text[16], fd_text[16], closed_text[16];
            snprintf(pid_text, sizeof(pid_text), "%u", (unsigned)getpid());
            snprintf(fd_text, sizeof(fd_text), "%d", fd);
            snprintf(closed_text, sizeof(closed_text), "%d", closed);
            require(lseek(fd, 4, SEEK_SET) == 4, "exec descriptor offset");
            char *args[] = {argv[0], "exec", pid_text, fd_text, closed_text, NULL};
            char *environment[] = {"EXEC_PROBE=retained", NULL};
            struct lume_run_request request = {.path = (uintptr_t)argv[0],
                                               .argv = (uintptr_t)args,
                                               .argc = 5,
                                               .envp = (uintptr_t)environment,
                                               .envc = 1};
            (void)native(LUME_SYS_EXEC, (uintptr_t)&request, 0, 0);
            require(0, "exec returned");
        }
        int status;
        require(waitpid((pid_t)pid, &status, 0) == pid && WIFEXITED(status) &&
                    WEXITSTATUS(status) == 23,
                "child exit status");
        require(shared_value == 17 && stack_value == 29, "parent data isolation");
        for (unsigned i = 0; i < 200000; ++i)
            require(heap[i] == (unsigned char)(i * 13), "parent heap isolation");
        require(lseek(fd, 0, SEEK_CUR) == 6 && !lseek(fd, 0, SEEK_SET),
                "inherited offset after exec");
    }
    pthread_t worker;
    require(!pthread_create(&worker, NULL, thread, NULL), "thread creation");
    while (!__atomic_load_n(&waiting, __ATOMIC_ACQUIRE))
        (void)native(LUME_SYS_SLEEP, 1, 0, 0);
    require(native(LUME_SYS_FORK, 0, 0, 0) == -LUME_EBUSY, "multithreaded fork rejection");
    __atomic_store_n(&done, 1, __ATOMIC_RELEASE);
    require(!pthread_join(worker, NULL), "thread join");
    require(!close(fd) && !close(closed) && !unlink("/data/process-probe.tmp"), "file cleanup");
    free(heap);
    puts("PROCESS_FORK_EXEC_RANDOM_PASS");
    return 0;
}
