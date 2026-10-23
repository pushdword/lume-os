#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include "lume/abi.h"
#include "lume/process.h"
#include "lume/service.h"
#include "lume/io.h"
#include "lume/net_commands.h"

static int64_t invoke(uint64_t number, uint64_t a, uint64_t b, uint64_t c)
{
    register uint64_t x8 __asm__("x8") = number, x0 __asm__("x0") = a, x1 __asm__("x1") = b,
                         x2 __asm__("x2") = c;
    __asm__ volatile("svc #0x4c55" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2) : "memory", "cc");
    return (int64_t)x0;
}

static int features(struct lume_system_features *out)
{
    return (int)invoke(LUME_SYS_FEATURES, (uintptr_t)out, sizeof(*out), LUME_FEATURE_VERSION);
}

#define CHECK(x)                                                                                   \
    do                                                                                             \
    {                                                                                              \
        if (!(x))                                                                                  \
        {                                                                                          \
            printf("SDK_TEST_FAIL line=%u errno=%d\n", __LINE__, errno);                           \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)

int main(int argc, char **argv)
{
    struct lume_system_features info;
    CHECK(!features(&info));
    CHECK(info.abi == LUME_ABI_VERSION && info.pointer_bits == 64 &&
          info.compat_pointer_bits == 32);
    CHECK(info.ddk_version == 1 && (info.features & LUME_FEATURE_SPAWN_ACTIONS));
    if (argc == 2 && !strcmp(argv[1], "--child"))
    {
        CHECK(getenv("SDK_TEST_CHILD") && !strcmp(getenv("SDK_TEST_CHILD"), "1"));
        CHECK(fcntl(3, F_GETFD) == 0);
        CHECK(write(1, "child-out\n", 10) == 10);
        CHECK(write(2, "child-err\n", 10) == 10);
        CHECK(write(3, "shared-fd\n", 10) == 10);
        return 7;
    }
    if (argc == 2 && !strcmp(argv[1], "--cap-leaf"))
    {
        CHECK(info.capabilities == 0);
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "--caps"))
    {
        CHECK(!(info.capabilities & LUME_CAP_SERVICE));
        CHECK(!invoke(LUME_SYS_CAP_DROP, LUME_CAP_NET_ADMIN | LUME_CAP_BOOT_UPDATE, 0, 0));
        CHECK(!features(&info) && info.capabilities == 0);
        CHECK(invoke(LUME_SYS_CAP_DROP, UINT64_MAX, 0, 0) == -LUME_EINVAL);
        CHECK(invoke(LUME_SYS_NET, LUME_NET_SCAN, UINT64_MAX, 0) == -LUME_EPERM);
        CHECK(invoke(LUME_SYS_NETCTL, LUME_NET_ADMIN, UINT64_MAX, 0) == -LUME_EPERM);
        CHECK(invoke(LUME_SYS_BOOT, LUME_BOOT_BEGIN, UINT64_MAX, 0) == -LUME_EPERM);
        struct lume_net_service_status status = {0};
        CHECK(invoke(LUME_SYS_SERVICE, LUME_SERVICE_PUBLISH, (uintptr_t)&status, sizeof(status)) ==
              -LUME_EPERM);
        struct lume_service_message message = {.version = 1, .command = NETD_RECONNECT};
        CHECK(invoke(LUME_SYS_SERVICE, LUME_SERVICE_SEND, (uintptr_t)&message, sizeof(message)) ==
              -LUME_EPERM);
        pid_t child;
        char *arguments[] = {argv[0], "--cap-leaf", NULL};
        CHECK(!posix_spawn(&child, argv[0], NULL, NULL, arguments, NULL));
        int code;
        CHECK(waitpid(child, &code, 0) == child && WIFEXITED(code) && WEXITSTATUS(code) == 0);
        return 0;
    }
    CHECK(info.capabilities == (LUME_CAP_NET_ADMIN | LUME_CAP_BOOT_UPDATE));
    int descriptors[2];
    CHECK(!pipe(descriptors));
    int duplicated = dup(descriptors[0]);
    CHECK(duplicated >= 0 && dup2(duplicated, 10) == 10);
    CHECK(!close(duplicated) && !close(10));
    CHECK((fcntl(descriptors[0], F_GETFL) & O_ACCMODE) == O_RDONLY);
    CHECK((fcntl(descriptors[1], F_GETFL) & O_ACCMODE) == O_WRONLY);
    CHECK(!fcntl(descriptors[0], F_SETFL, O_NONBLOCK));
    char byte;
    CHECK(read(descriptors[0], &byte, 1) == -1 && errno == EAGAIN);
    struct pollfd polled = {.fd = descriptors[0], .events = POLLIN};
    CHECK(!poll(&polled, 1, 0));
    CHECK(write(descriptors[1], "q", 1) == 1);
    CHECK(poll(&polled, 1, 0) == 1 && (polled.revents & POLLIN));
    CHECK(read(descriptors[0], &byte, 1) == 1 && byte == 'q');
    CHECK(!close(descriptors[0]) && !close(descriptors[1]));
    CHECK(invoke(LUME_SYS_FEATURES, (uintptr_t)&info, sizeof(info) - 1, LUME_FEATURE_VERSION) ==
          -LUME_EINVAL);
    CHECK(invoke(LUME_SYS_FEATURES, UINT64_MAX, sizeof(info), LUME_FEATURE_VERSION) ==
          -LUME_EFAULT);
    CHECK(invoke(LUME_SYS_RUN_ACTIONS, UINT64_MAX, 0, 0) == -LUME_EFAULT);
    int output = open("/data/ddk/posix-out", O_CREAT | O_TRUNC | O_WRONLY, 0600);
    CHECK(output == 3 && !fcntl(output, F_SETFD, FD_CLOEXEC));
    CHECK(!fcntl(output, F_SETFL, O_APPEND) && (fcntl(output, F_GETFL) & O_APPEND));
    CHECK(!fcntl(output, F_SETFL, 0) && !(fcntl(output, F_GETFL) & O_APPEND));
    posix_spawn_file_actions_t actions;
    CHECK(!posix_spawn_file_actions_init(&actions));
    CHECK(!posix_spawn_file_actions_adddup2(&actions, output, output));
    CHECK(!posix_spawn_file_actions_adddup2(&actions, output, 1));
    char error_path[] = "/data/ddk/posix-err";
    CHECK(!posix_spawn_file_actions_addopen(&actions, 2, error_path, O_CREAT | O_TRUNC | O_WRONLY,
                                            0600));
    error_path[0] = 'X';
    char *arguments[] = {argv[0], "--child", NULL};
    char *environment[] = {"SDK_TEST_CHILD=1", NULL};
    pid_t child;
    CHECK(!posix_spawn(&child, argv[0], &actions, NULL, arguments, environment));
    CHECK(!posix_spawn_file_actions_destroy(&actions));
    int code;
    CHECK(waitpid(child, &code, 0) == child && WIFEXITED(code) && WEXITSTATUS(code) == 7);
    CHECK(fcntl(output, F_GETFD) == FD_CLOEXEC);
    CHECK(write(output, "parent-alive\n", 13) == 13);
    CHECK(!close(output));
    for (unsigned i = 0; i < 12; ++i)
    {
        CHECK(!posix_spawn_file_actions_init(&actions));
        CHECK(!posix_spawn_file_actions_addclose(&actions, 15));
        CHECK(posix_spawn(&child, argv[0], &actions, NULL, arguments, environment) == EBADF);
        CHECK(!posix_spawn_file_actions_destroy(&actions));
    }
    CHECK(!posix_spawn_file_actions_init(&actions));
    CHECK(!posix_spawn_file_actions_addopen(&actions, 1, "/missing/sdk-output", O_WRONLY, 0600));
    CHECK(posix_spawn(&child, argv[0], &actions, NULL, arguments, environment) == ENOENT);
    CHECK(!posix_spawn_file_actions_destroy(&actions));
    arguments[1] = "--caps";
    CHECK(!posix_spawn(&child, argv[0], NULL, NULL, arguments, NULL));
    CHECK(waitpid(child, &code, 0) == child && WIFEXITED(code) && WEXITSTATUS(code) == 0);
    CHECK(!features(&info) && info.capabilities == (LUME_CAP_NET_ADMIN | LUME_CAP_BOOT_UPDATE));
    puts("SDK_POSIX_SPAWN_CAPABILITIES_PASS");
    return 0;
}
