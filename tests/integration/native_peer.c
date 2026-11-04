/* SPDX-License-Identifier: BSD-3-Clause */
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <lume/abi.h>
#include <lume/socket.h>
#include <lume/syscall_native.h>
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/uio.h>

static void peer(int fd, uid_t uid, gid_t gid)
{
    uid_t actual_uid = 65535;
    gid_t actual_gid = 65535;
    assert(!getpeereid(fd, &actual_uid, &actual_gid));
    assert(actual_uid == uid && actual_gid == gid);
}

static void rights_vectors(void)
{
    int sockets[2];
    assert(!socketpair(AF_UNIX, SOCK_STREAM, 0, sockets));
    int original = open("/tmp/rights-vectors", O_CREAT | O_EXCL | O_RDWR, 0600);
    assert(original >= 0);
    assert(write(original, "DATA", 4) == 4 && !lseek(original, 0, SEEK_SET));

    union
    {
        struct cmsghdr alignment;
        unsigned char bytes[CMSG_SPACE(sizeof(int))];
    } control = {0};
    struct iovec vectors[] = {{NULL, 0}, {(void *)"header", 6}, {(void *)"body", 4}};
    struct msghdr message = {.msg_iov = vectors,
                             .msg_iovlen = 3,
                             .msg_control = control.bytes,
                             .msg_controllen = sizeof(control.bytes)};
    struct cmsghdr *header = CMSG_FIRSTHDR(&message);
    header->cmsg_level = SOL_SOCKET;
    header->cmsg_type = SCM_RIGHTS;
    header->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(header), &original, sizeof(original));
    ssize_t sent = sendmsg(sockets[0], &message, 0);
    assert(sent > 0 && sent <= 10);
    assert(!close(original));
    if (sent < 10)
        assert(write(sockets[0], "headerbody" + sent, 10 - sent) == 10 - sent);
    char payload[16];
    struct iovec input = {payload, sizeof(payload)};
    struct msghdr received = {.msg_iov = &input,
                              .msg_iovlen = 1,
                              .msg_control = control.bytes,
                              .msg_controllen = sizeof(control.bytes)};
    assert(recvmsg(sockets[1], &received, 0) == 10 && !memcmp(payload, "headerbody", 10));
    header = CMSG_FIRSTHDR(&received);
    assert(header && header->cmsg_type == SCM_RIGHTS && !received.msg_flags);
    int imported;
    memcpy(&imported, CMSG_DATA(header), sizeof(imported));
    assert(read(imported, payload, sizeof(payload)) == 4 && !memcmp(payload, "DATA", 4));
    assert(!lseek(imported, 0, SEEK_SET));
    assert(!fcntl(sockets[0], F_SETFL, O_NONBLOCK));
    char full[4096];
    memset(full, 'x', sizeof(full));
    assert(write(sockets[0], full, sizeof(full)) == sizeof(full));
    header = CMSG_FIRSTHDR(&message);
    memcpy(CMSG_DATA(header), &imported, sizeof(imported));
    assert(sendmsg(sockets[0], &message, 0) == -1 && errno == EAGAIN);
    assert(read(sockets[1], payload, 1) == 1 && payload[0] == 'x');
    assert(sendmsg(sockets[0], &message, 0) > 0);
    assert(read(sockets[1], full, sizeof(full) - 1) == sizeof(full) - 1);
    received.msg_controllen = sizeof(control.bytes);
    assert(recvmsg(sockets[1], &received, 0) == 1 && payload[0] == 'h');
    header = CMSG_FIRSTHDR(&received);
    assert(header && header->cmsg_type == SCM_RIGHTS && !received.msg_flags);
    int second;
    memcpy(&second, CMSG_DATA(header), sizeof(second));
    assert(read(second, payload, sizeof(payload)) == 4 && !memcmp(payload, "DATA", 4));
    assert(!close(second) && !close(imported));
    assert(!close(sockets[0]) && !close(sockets[1]));
    assert(!unlink("/tmp/rights-vectors"));
    puts("RIGHTS_VECTORS_PASS");
}

static void daemon_check(void)
{
    const char *path = "/data/daemon-check";
    int ready[2];
    assert(!pipe(ready));
    pid_t child = fork();
    assert(child >= 0);
    if (!child)
    {
        assert(!close(ready[0]));
        assert(!daemon(0, 0));
        char directory[8], value;
        assert(getsid(0) == getpid());
        assert(getcwd(directory, sizeof(directory)) && !strcmp(directory, "/"));
        assert(read(0, &value, 1) == 0 && write(1, "x", 1) == 1 && write(2, "x", 1) == 1);
        int output = open(path, O_CREAT | O_EXCL | O_WRONLY, 0600);
        assert(output >= 0 && write(output, "ok", 2) == 2 && !close(output));
        assert(write(ready[1], "r", 1) == 1 && !close(ready[1]));
        _exit(0);
    }
    int status;
    assert(!close(ready[1]));
    assert(waitpid(child, &status, 0) == child && WIFEXITED(status) && !WEXITSTATUS(status));
    char receipt;
    assert(read(ready[0], &receipt, 1) == 1 && receipt == 'r');
    assert(read(ready[0], &receipt, 1) == 0 && !close(ready[0]));
    int input = open(path, O_RDONLY);
    char value[2];
    assert(input >= 0 && read(input, value, sizeof(value)) == sizeof(value));
    assert(!memcmp(value, "ok", sizeof(value)));
    assert(!close(input));
    assert(!unlink(path));
    puts("DAEMON_SESSION_REDIRECTION_PASS");
}

int main(void)
{
    assert(!geteuid() && !getegid());
    int descriptors[2];
    assert(!socketpair(AF_UNIX, SOCK_STREAM, 0, descriptors));
    assert(!setegid(45) && !seteuid(1001));
    peer(descriptors[0], 0, 0);
    assert(!seteuid(0) && !setegid(0));
    assert(!close(descriptors[1]));
    peer(descriptors[0], 0, 0);
    struct lume_socket_request query = {.version = LUME_SOCKET_VERSION,
                                        .bytes = sizeof(query),
                                        .fd = descriptors[0],
                                        .count = sizeof(struct lume_socket_credentials)};
    assert(lume_native_call(LUME_SYS_SOCKET, LUME_SOCKET_CREDENTIALS, (uintptr_t)&query,
                            sizeof(query)) == -LUME_EFAULT);
    assert(!close(descriptors[0]));
    puts("PEER_PAIR_SNAPSHOT_PASS");

    int listener = socket(AF_UNIX, SOCK_STREAM, 0);
    assert(listener >= 0);
    struct sockaddr_un address = {.sun_family = AF_UNIX, .sun_path = "/tmp/peer-check"};
    socklen_t bytes = offsetof(struct sockaddr_un, sun_path) + sizeof("/tmp/peer-check");
    assert(!bind(listener, (struct sockaddr *)&address, bytes));
    assert(!listen(listener, 1));
    pid_t child = fork();
    assert(child >= 0);
    if (!child)
    {
        assert(!close(listener));
        assert(!setegid(45) && !seteuid(1001));
        int client = socket(AF_UNIX, SOCK_STREAM, 0);
        assert(client >= 0);
        assert(!connect(client, (struct sockaddr *)&address, bytes));
        peer(client, 0, 0);
        assert(!seteuid(0) && !setegid(0));
        assert(write(client, "c", 1) == 1 && !close(client));
        _exit(0);
    }
    int connection = accept(listener, NULL, NULL);
    assert(connection >= 0);
    char value;
    assert(read(connection, &value, 1) == 1 && value == 'c');
    assert(read(connection, &value, 1) == 0);
    peer(connection, 1001, 45);
    int status;
    assert(waitpid(child, &status, 0) == child && WIFEXITED(status) && !WEXITSTATUS(status));
    assert(!close(connection) && !close(listener));
    uid_t uid = 1234;
    gid_t gid = 5678;
    assert(getpeereid(-1, &uid, &gid) == -1 && errno == EBADF);
    assert(uid == 1234 && gid == 5678);
    puts("PEER_NAMED_FORK_PASS");
    char storage[128];
    struct group group, *result;
    errno = EINTR;
    assert(!getgrnam_r("root", &group, storage + 1, sizeof(storage) - 1, &result));
    assert(result == &group && !group.gr_gid && !strcmp(group.gr_name, "root"));
    assert(group.gr_mem && !strcmp(group.gr_mem[0], "root") && !group.gr_mem[1]);
    assert((uintptr_t)group.gr_mem % _Alignof(char *) == 0 && errno == EINTR);
    assert(getgrgid_r(0, &group, storage, 1, &result) == ERANGE && !result);
    assert(!getgrgid_r(12345, &group, storage, sizeof(storage), &result) && !result);
    setgrent();
    result = getgrent();
    assert(result && !strcmp(result->gr_name, "root") && !result->gr_gid);
    assert(!getgrent() && !getgrent());
    setgrent();
    assert(getgrent());
    endgrent();
    assert(!getgrent());
    setpwent();
    struct passwd *account = getpwent();
    assert(account && !strcmp(account->pw_name, "root") && !account->pw_uid);
    assert(!getpwent());
    setpwent();
    assert(getpwent());
    endpwent();
    assert(!getpwent());
    gid_t saved[NGROUPS_MAX], groups[NGROUPS_MAX], actual[NGROUPS_MAX];
    int group_count = getgroups(NGROUPS_MAX, saved);
    assert(group_count >= 0 && group_count <= NGROUPS_MAX);
    for (unsigned i = 0; i < NGROUPS_MAX; ++i)
        groups[i] = i + 10;
    assert(!setgroups(NGROUPS_MAX, groups));
    assert(getgroups(0, NULL) == NGROUPS_MAX);
    assert(getgroups(NGROUPS_MAX, actual) == NGROUPS_MAX);
    assert(!memcmp(actual, groups, sizeof(groups)));
    assert(getgroups(NGROUPS_MAX - 1, actual) == -1 && errno == EINVAL);
    assert(setgroups(NGROUPS_MAX + 1, groups) == -1 && errno == EINVAL);
    assert(!setgroups(group_count, saved));
    struct timespec resolution, now;
    assert(!clock_getres(CLOCK_MONOTONIC, &resolution));
    assert(resolution.tv_sec == 0 && resolution.tv_nsec == 1000000);
    assert(!clock_gettime(CLOCK_MONOTONIC, &now) && now.tv_nsec % resolution.tv_nsec == 0);
    assert(!clock_getres(CLOCK_REALTIME, NULL));
    assert(clock_getres((clockid_t)-1, NULL) == -1 && errno == EINVAL);
    puts("SESSION_ACCOUNTS_CLOCK_PASS");
    daemon_check();
    rights_vectors();
    puts("PEER_NATIVE_PASS");
    return 0;
}
