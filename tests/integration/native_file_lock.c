#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

static int change(int fd, int type)
{
    struct flock lock = {
        .l_type = type, .l_whence = SEEK_SET, .l_start = INT64_C(0x100001030), .l_len = 9};
    return fcntl(fd, F_OFD_SETLK, &lock);
}

static int blocked(int fd)
{
    errno = 0;
    return change(fd, F_WRLCK) == -1 && errno == EAGAIN;
}

static _Noreturn void child(int inherited, int duplicate, int command, int reply, const char *path)
{
    struct flock lock = {
        .l_type = F_WRLCK, .l_whence = SEEK_SET, .l_start = INT64_C(0x100001030), .l_len = 9};
    if (fcntl(inherited, F_OFD_GETLK, &lock) || lock.l_type != F_UNLCK)
        _exit(10);
    int independent = open(path, O_RDWR);
    if (independent < 0 || !blocked(independent))
        _exit(11);
    if (close(inherited) || close(duplicate) || write(reply, "r", 1) != 1)
        _exit(12);
    char signal;
    if (read(command, &signal, 1) != 1 || signal != 'd' || !blocked(independent) ||
        write(reply, "c", 1) != 1)
        _exit(13);
    if (read(command, &signal, 1) != 1 || signal != 'f' || change(independent, F_WRLCK) ||
        write(reply, "x", 1) != 1)
        _exit(14);
    _exit(0);
}

int main(void)
{
    _Static_assert(sizeof(((struct flock *)0)->l_start) == 8, "lock probe requires LP64 offsets");
    int probe = open("/dev/null", O_RDWR);
    struct flock lock = {.l_type = F_WRLCK, .l_whence = SEEK_SET};
    assert(probe >= 0 && fcntl(probe, F_OFD_GETLK, &lock) == 0 && lock.l_type == F_UNLCK);
    assert(close(probe) == 0);
    const char *path = "/tmp/ofd-lock-probe";
    int fd = open(path, O_CREAT | O_TRUNC | O_RDWR, 0600);
    assert(fd >= 3 && change(fd, F_WRLCK) == 0);
    int duplicate = dup(fd), commands[2], replies[2];
    assert(duplicate >= 3 && pipe(commands) == 0 && pipe(replies) == 0);
    pid_t pid = fork();
    assert(pid >= 0);
    if (!pid)
    {
        if (close(commands[1]) || close(replies[0]))
            _exit(9);
        child(fd, duplicate, commands[0], replies[1], path);
    }
    assert(close(commands[0]) == 0 && close(replies[1]) == 0);
    char signal;
    assert(read(replies[0], &signal, 1) == 1 && signal == 'r');
    assert(close(fd) == 0);
    assert(write(commands[1], "d", 1) == 1);
    assert(read(replies[0], &signal, 1) == 1 && signal == 'c');
    assert(close(duplicate) == 0);
    assert(write(commands[1], "f", 1) == 1);
    assert(read(replies[0], &signal, 1) == 1 && signal == 'x');
    int status;
    assert(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0);
    assert(close(commands[1]) == 0 && close(replies[0]) == 0);
    fd = open(path, O_RDWR);
    assert(fd >= 3 && change(fd, F_WRLCK) == 0);
    assert(close(fd) == 0 && unlink(path) == 0);
    puts("NATIVE_OFD_LOCK_PASS");
    return 0;
}
