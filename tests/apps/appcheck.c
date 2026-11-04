/* SPDX-License-Identifier: BSD-3-Clause */
#include "file_operations.h"
#include "lume/syscall_native.h"
#include "lume/abi.h"
#include "lume/process.h"
#include "lume/realtime_abi.h"
#include <assert.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/un.h>
#include <time.h>

int main(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "unset"))
    {
        struct timespec now;
        assert(!clock_gettime(CLOCK_MONOTONIC, &now));
        assert(clock_gettime(CLOCK_REALTIME, &now) == -1 && errno == EAGAIN);
        struct timeval utc;
        assert(gettimeofday(&utc, NULL) == -1 && errno == EAGAIN);
        puts("APP_UNSET_CLOCK_PASS");
        return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "dns"))
    {
        struct addrinfo *address;
        int result = getaddrinfo(argv[2], "443", NULL, &address);
        if (result)
        {
            fprintf(stderr, "RESOLVER_FAIL %d\n", result);
            return 1;
        }
        char text[INET_ADDRSTRLEN];
        assert(inet_ntop(AF_INET, &((struct sockaddr_in *)address->ai_addr)->sin_addr, text,
                         sizeof(text)));
        printf("RESOLVER_PASS %s %s\n", argv[2], text);
        freeaddrinfo(address);
        return 0;
    }
    struct timespec before, after, utc;
    assert(!clock_gettime(CLOCK_MONOTONIC, &before));
    struct lume_realtime_request request = {
        .version = LUME_REALTIME_VERSION, .bytes = sizeof(request), .seconds = 1793628000};
    assert(!lume_native_call(LUME_SYS_REALTIME, LUME_REALTIME_SET, (uintptr_t)&request,
                             sizeof(request)));
    assert(!clock_gettime(CLOCK_REALTIME, &utc));
    assert(utc.tv_sec >= 1793628000 && utc.tv_sec < 1793628010);
    struct timeval now;
    assert(!gettimeofday(&now, NULL) && now.tv_sec >= utc.tv_sec);
    assert(!clock_gettime(CLOCK_MONOTONIC, &after));
    assert(after.tv_sec >= before.tv_sec && after.tv_sec - before.tv_sec < 10);
    pid_t child = fork();
    assert(child >= 0);
    if (!child)
    {
        assert(!lume_native_call(LUME_SYS_CAP_DROP, LUME_CAP_CLOCK_ADMIN, 0, 0));
        _exit(lume_native_call(LUME_SYS_REALTIME, LUME_REALTIME_SET, (uintptr_t)&request,
                               sizeof(request)) == -LUME_EACCES
                  ? 0
                  : 1);
    }
    int status;
    assert(waitpid(child, &status, 0) == child && WIFEXITED(status) && !WEXITSTATUS(status));
    puts("APP_CLOCK_PASS");
    struct addrinfo *address;
    assert(!getaddrinfo("127.0.0.1", "80", NULL, &address));
    freeaddrinfo(address);
    struct addrinfo hints = {.ai_family = AF_INET, .ai_flags = AI_NUMERICHOST};
    assert(getaddrinfo("example.org", "80", &hints, &address) == EAI_NONAME);
    puts("APP_RESOLVER_PASS");
    int pair[2];
    assert(!pipe(pair));
    assert(write(pair[1], "x", 1) == 1);
    fd_set reads;
    FD_ZERO(&reads);
    FD_SET(pair[0], &reads);
    struct timeval immediately = {0};
    assert(select(FD_SETSIZE, &reads, NULL, NULL, &immediately) == 1);
    assert(FD_ISSET(pair[0], &reads));
    assert(!close(pair[0]) && !close(pair[1]));
    FD_ZERO(&reads);
    FD_SET(FD_SETSIZE - 1, &reads);
    assert(select(FD_SETSIZE, &reads, NULL, NULL, &immediately) == -1 && errno == EBADF);
    puts("APP_SELECT_PASS");
    int sockets[20];
    struct sockaddr_un local = {.sun_family = AF_UNIX};
    for (unsigned i = 0; i < 20; ++i)
    {
        sockets[i] = socket(AF_UNIX, SOCK_STREAM, 0);
        assert(sockets[i] >= 0);
        snprintf(local.sun_path, sizeof(local.sun_path), "/data/apps-socket-%ld-%u", (long)getpid(),
                 i);
        assert(!bind(sockets[i], (const struct sockaddr *)&local, sizeof(local)));
        assert(!listen(sockets[i], 1));
    }
    int duplicate = socket(AF_UNIX, SOCK_STREAM, 0);
    assert(duplicate >= 0);
    assert(bind(duplicate, (const struct sockaddr *)&local, sizeof(local)) == -1 &&
           errno == EADDRINUSE);
    assert(!close(duplicate));
    for (unsigned i = 0; i < 20; ++i)
        assert(!close(sockets[i]));
    int pairs[12][2];
    for (unsigned i = 0; i < 12; ++i)
        assert(!socketpair(AF_UNIX, SOCK_STREAM, 0, pairs[i]));
    for (unsigned i = 0; i < 12; ++i)
    {
        assert(write(pairs[i][0], "s", 1) == 1);
        char byte;
        assert(read(pairs[i][1], &byte, 1) == 1 && byte == 's');
        assert(!close(pairs[i][0]) && !close(pairs[i][1]));
    }
    puts("APP_SOCKET_CAPACITY_PASS");
    char directory[128], first[160], second[160], renamed[160];
    snprintf(directory, sizeof(directory), "/data/apps-%ld", (long)getpid());
    assert(!mkdir(directory, 0700));
    snprintf(first, sizeof(first), "%s/original", directory);
    snprintf(second, sizeof(second), "%s/copy", directory);
    snprintf(renamed, sizeof(renamed), "%s/renamed", directory);
    int fd = open(first, O_WRONLY | O_CREAT | O_EXCL, 0600);
    assert(fd >= 0);
    const char text[] = "olá, ação, avô\n";
    assert(write(fd, text, sizeof(text)) == sizeof(text) && !close(fd));
    assert(!file_copy(first, second));
    assert(file_copy(first, second) < 0 && errno == EEXIST);
    char copy[sizeof(text)];
    fd = open(second, O_RDONLY);
    assert(fd >= 0 && read(fd, copy, sizeof(copy)) == sizeof(copy) && !close(fd));
    assert(!memcmp(copy, text, sizeof(text)));
    assert(!rename(second, renamed));
    assert(!unlink(first) && !unlink(renamed) && !rmdir(directory));
    puts("APP_FILES_PASS");
    int barrier[2];
    assert(!pipe(barrier));
    pid_t children[20];
    pid_t highest = 0;
    for (unsigned i = 0; i < 20; ++i)
    {
        children[i] = fork();
        assert(children[i] >= 0);
        if (!children[i])
        {
            close(barrier[1]);
            char value;
            _exit(read(barrier[0], &value, 1) == 0 ? 0 : 1);
        }
        if (children[i] > highest)
            highest = children[i];
    }
    assert(highest > 16);
    close(barrier[0]);
    close(barrier[1]);
    for (unsigned i = 0; i < 20; ++i)
        assert(waitpid(children[i], &status, 0) == children[i] && WIFEXITED(status) &&
               !WEXITSTATUS(status));
    puts("APP_TASKS_PASS");
    puts("APPLICATION_NATIVE_PASS");
    return 0;
}
