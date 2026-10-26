#include "lume/abi.h"
#include "lume/credential.h"
#include "lume/process.h"
#include "lume/socket.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

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
        fprintf(stderr, "CREDENTIAL_PROBE_FAIL %s\n", name);
        exit(1);
    }
}

static int credential(unsigned operation, struct lume_credential_request *request)
{
    request->version = LUME_CREDENTIAL_VERSION;
    request->bytes = sizeof(*request);
    return (int)native(LUME_SYS_CREDENTIAL, operation, (uintptr_t)request, sizeof(*request));
}

static int limit(unsigned operation, unsigned resource, uint64_t soft, uint64_t hard)
{
    struct lume_limit_request request = {.version = LUME_CREDENTIAL_VERSION,
                                         .bytes = sizeof(request),
                                         .resource = resource,
                                         .soft = soft,
                                         .hard = hard};
    return (int)native(LUME_SYS_CREDENTIAL, operation, (uintptr_t)&request, sizeof(request));
}

static void check_child(int success, unsigned code)
{
    if (!success)
    {
        (void)native(LUME_SYS_EXIT, code, 0, 0);
        __builtin_trap();
    }
}

int main(void)
{
    require(!mkdir("/data/credential-jail", 0700), "empty jail creation");
    int file = open("/data/credential-secret", O_RDWR | O_CREAT | O_EXCL, 0600);
    require(file >= 0 && write(file, "SECRET", 6) == 6 && !lseek(file, 0, SEEK_SET),
            "secret creation");
    struct lume_socket_request pair = {
        .version = LUME_SOCKET_VERSION, .bytes = sizeof(pair), .family = 1, .type = 1};
    require(!native(LUME_SYS_SOCKET, LUME_SOCKET_PAIR, (uintptr_t)&pair, sizeof(pair)),
            "IPC setup");
    char byte = 'R';
    struct lume_socket_request send = {.version = LUME_SOCKET_VERSION,
                                       .bytes = sizeof(send),
                                       .fd = pair.fd,
                                       .other = file,
                                       .buffer = (uintptr_t)&byte,
                                       .count = 1};
    require(native(LUME_SYS_SOCKET, LUME_SOCKET_SEND_FD, (uintptr_t)&send, sizeof(send)) == 1,
            "queued descriptor");
    int64_t pid = native(LUME_SYS_FORK, 0, 0, 0);
    require(pid >= 0, "fork");
    if (!pid)
    {
        struct lume_credential_request request = {.path = (uintptr_t)"/data/credential-jail"};
        check_child(!credential(LUME_CREDENTIAL_CHROOT, &request), 11);
        request = (struct lume_credential_request){.gid = 65534, .egid = 65534, .sgid = 65534};
        check_child(!credential(LUME_CREDENTIAL_GID, &request), 12);
        check_child(!limit(LUME_CREDENTIAL_LIMIT_SET, LUME_LIMIT_FILES, 0, 0), 13);
        check_child(!limit(LUME_CREDENTIAL_LIMIT_SET, LUME_LIMIT_PROCESSES, 0, 0), 14);
        request = (struct lume_credential_request){.uid = 65534, .euid = 65534, .suid = 65534};
        check_child(!credential(LUME_CREDENTIAL_UID, &request), 15);
        request = (struct lume_credential_request){0};
        check_child(!credential(LUME_CREDENTIAL_GET, &request) && request.uid == 65534 &&
                        request.euid == 65534 && request.gid == 65534,
                    16);
        char secret;
        check_child(native(LUME_SYS_READ, (unsigned)file, (uintptr_t)&secret, 1) == -LUME_EACCES,
                    17);
        check_child(
            native(LUME_SYS_OPEN, (uintptr_t)"/data/credential-secret", 0, 0) == -LUME_EMFILE, 18);
        check_child(native(LUME_SYS_CHDIR, (uintptr_t)"/", 0, 0) == 0, 19);
        check_child(native(LUME_SYS_CHDIR, (uintptr_t)"/../../data", 0, 0) < 0, 20);
        request = (struct lume_credential_request){.uid = 0, .euid = 0, .suid = 0};
        check_child(credential(LUME_CREDENTIAL_UID, &request) == -LUME_EPERM, 21);
        check_child(limit(LUME_CREDENTIAL_LIMIT_SET, LUME_LIMIT_FILES, 16, 16) == -LUME_EPERM, 22);
        check_child(native(LUME_SYS_FORK, 0, 0, 0) == -LUME_EAGAIN, 23);
        struct lume_socket_request receive = {.version = LUME_SOCKET_VERSION,
                                              .bytes = sizeof(receive),
                                              .fd = pair.other,
                                              .buffer = (uintptr_t)&byte,
                                              .count = 1};
        check_child(native(LUME_SYS_SOCKET, LUME_SOCKET_RECEIVE_FD, (uintptr_t)&receive,
                           sizeof(receive)) == -LUME_EMFILE,
                    24);
        check_child(read(pair.other, &byte, 1) == 1 && byte == 'R', 25);
        check_child(write(pair.other, "P", 1) == 1, 26);
        struct lume_system_features features;
        check_child(!native(LUME_SYS_FEATURES, (uintptr_t)&features, sizeof(features),
                            LUME_FEATURE_VERSION) &&
                        !features.capabilities,
                    27);
        (void)native(LUME_SYS_EXIT, 0, 0, 0);
        __builtin_trap();
    }
    int status;
    require(waitpid((pid_t)pid, &status, 0) == pid, "child wait");
    if (WEXITSTATUS(status))
        fprintf(stderr, "CREDENTIAL_CHILD_STATUS=%d\n", WEXITSTATUS(status));
    require(WIFEXITED(status) && !WEXITSTATUS(status), "credential isolation");
    require(read(pair.fd, &byte, 1) == 1 && byte == 'P', "IPC after credential drop");
    struct lume_credential_request identity = {0};
    require(!credential(LUME_CREDENTIAL_GET, &identity) && !identity.uid && !identity.euid,
            "parent credentials unchanged");
    require(!close(pair.fd) && !close(pair.other) && !close(file), "descriptor release");
    require(!unlink("/data/credential-secret") && !rmdir("/data/credential-jail"),
            "filesystem cleanup");
    puts("CREDENTIAL_JAIL_LIMITS_PASS");
    return 0;
}
