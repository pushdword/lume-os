/* SPDX-License-Identifier: BSD-3-Clause */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pty.h>
#include <spawn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

extern char **environ;
static const char *binary_directory;
static unsigned checks;

static void run(const char *name, const char *const arguments[], const char *input_path,
                const void *input, size_t input_bytes, const void *expected, size_t expected_bytes,
                int status)
{
    char path[1024];
    int length = snprintf(path, sizeof(path), "%s/%s", binary_directory, name);
    assert(length > 0 && (size_t)length < sizeof(path));
    int in[2] = {-1, -1}, out[2];
    if (input_path)
    {
        in[0] = open(input_path, O_RDONLY);
        assert(in[0] >= 0);
    }
    else
    {
        assert(pipe(in) == 0);
    }
    assert(pipe(out) == 0);
    posix_spawn_file_actions_t actions;
    assert(posix_spawn_file_actions_init(&actions) == 0);
    assert(posix_spawn_file_actions_adddup2(&actions, in[0], 0) == 0);
    assert(posix_spawn_file_actions_adddup2(&actions, out[1], 1) == 0);
    assert(posix_spawn_file_actions_addclose(&actions, in[0]) == 0);
    if (in[1] >= 0)
    {
        assert(posix_spawn_file_actions_addclose(&actions, in[1]) == 0);
    }
    assert(posix_spawn_file_actions_addclose(&actions, out[0]) == 0);
    assert(posix_spawn_file_actions_addclose(&actions, out[1]) == 0);
    pid_t child;
    int error = posix_spawn(&child, path, &actions, NULL, (char *const *)arguments, environ);
    assert(posix_spawn_file_actions_destroy(&actions) == 0);
    assert(close(in[0]) == 0);
    assert(close(out[1]) == 0);
    if (error)
    {
        fprintf(stderr, "Cannot spawn %s: %s\n", path, strerror(error));
        exit(1);
    }
    size_t sent = 0;
    while (sent < input_bytes)
    {
        ssize_t count = write(in[1], (const char *)input + sent, input_bytes - sent);
        if (count < 0 && errno == EINTR)
        {
            continue;
        }
        assert(count > 0);
        sent += (size_t)count;
    }
    if (in[1] >= 0)
    {
        assert(close(in[1]) == 0);
    }
    char buffer[1024];
    size_t received = 0;
    int matches = 1;
    for (;;)
    {
        ssize_t count = read(out[0], buffer, sizeof(buffer));
        if (count < 0 && errno == EINTR)
        {
            continue;
        }
        assert(count >= 0);
        if (!count)
        {
            break;
        }
        if (received > expected_bytes || (size_t)count > expected_bytes - received ||
            memcmp(buffer, (const char *)expected + received, (size_t)count))
        {
            matches = 0;
        }
        received += (size_t)count;
    }
    assert(close(out[0]) == 0);
    int result = -1;
    pid_t waited;
    do
    {
        waited = waitpid(child, &result, 0);
    } while (waited < 0 && errno == EINTR);
    if (waited != child || !WIFEXITED(result) || WEXITSTATUS(result) != status || !matches ||
        received != expected_bytes)
    {
        fprintf(stderr, "FAIL coreutils %s status=%d bytes=%zu expected=%zu\n", name, result,
                received, expected_bytes);
        exit(1);
    }
    ++checks;
}

static void write_fixture(const char *path, const char *data, size_t bytes)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
    assert(fd >= 0);
    assert(write(fd, data, bytes) == (ssize_t)bytes);
    assert(close(fd) == 0);
}

static void check_terminal_descriptors(void)
{
    int fd = open("/dev/null", O_RDONLY);
    struct termios attributes;
    assert(fd >= 0);
    assert(!isatty(fd) && errno == ENOTTY);
    assert(tcgetattr(fd, &attributes) == -1 && errno == ENOTTY);
    assert(close(fd) == 0);
    assert(!isatty(fd) && errno == EBADF);
    assert(tcgetattr(fd, &attributes) == -1 && errno == EBADF);
    assert(ttyname(fd) == NULL && errno == EBADF);

    fd = open("/dev/console", O_RDONLY | O_NOCTTY);
    assert(fd >= 0 && isatty(fd));
    assert(close(fd) == 0);
    int master, slave;
    assert(openpty(&master, &slave, NULL, NULL, NULL) == 0);
    assert(isatty(master) && isatty(slave));
    assert(tcgetattr(slave, &attributes) == 0);
    assert(close(master) == 0);
    assert(close(slave) == 0);
    ++checks;
}

#define CHECK(name, input, output, status, ...)                                                    \
    run(name, (const char *const[]){name, __VA_ARGS__, NULL}, NULL, input, sizeof(input) - 1,      \
        output, sizeof(output) - 1, status)
#define CHECK_FILE_INPUT(name, path, output, status, ...)                                          \
    run(name, (const char *const[]){name, __VA_ARGS__, NULL}, path, NULL, 0, output,               \
        sizeof(output) - 1, status)
#define PAYLOAD_SHA256 "d4e4877bac978b7952f0d544fc52ebff5411d351d129f1f056fa43f11da9af2b"
#define EMPTY_SHA256 "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"

int main(int argc, char **argv)
{
    if (argc != 2 || argv[1][0] != '/')
    {
        fprintf(stderr, "Usage: %s /absolute/path/to/coreutils/bin\n", argv[0]);
        return 2;
    }
    binary_directory = argv[1];
    check_terminal_descriptors();
    CHECK("cat", "a\0b\n", "a\0b\n", 0, "-");
    CHECK("head", "abcdef", "abc", 0, "-c", "3");
    CHECK("tail", "abcdef", "def", 0, "-c", "3");
    CHECK("sort", "10\n2\n1\n", "1\n2\n10\n", 0, "-n");
    CHECK("comm", "1\n2\n3\n3\n4\n5\n6\n6\n7\n8\n9\n9\n",
          "1\n\t2\n\t\t3\n4\n\t5\n\t\t6\n7\n\t8\n\t\t9\n", 0, "-", "-");
    CHECK("uniq", "a\na\nb\n", "a\nb\n", 0, "-");
    CHECK("cut", "a:b:c\n", "b\n", 0, "-d:", "-f2");
    CHECK("tr", "abc", "ABC", 0, "a-z", "A-Z");
    CHECK("base64", "abc", "YWJj\n", 0, "-");
    CHECK("base64", "YWJj\n", "abc", 0, "-d");
    CHECK("md5sum", "abc", "900150983cd24fb0d6963f7d28e17f72  -\n", 0, "-");
    CHECK("sha256sum", "abc",
          "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad  -\n", 0, "-");
    CHECK("seq", "", "1\n2\n3\n", 0, "1", "3");
    CHECK("basename", "", "file\n", 0, "/a/file");
    CHECK("dirname", "", "/a\n", 0, "/a/file");
    CHECK("date", "", "1970-01-01 00001 000000000\n", 0, "-u", "-d", "@0", "+%F %5m %N");
    CHECK("test", "", "", 0, "42", "-eq", "42");
    CHECK("test", "", "", 1, "42", "-eq", "43");
    CHECK("[", "", "", 0, "x", "=", "x", "]");
    CHECK("true", "", "", 0, "--");
    CHECK("false", "", "", 1, "--");

    char scratch[] = "/tmp/coreutils-XXXXXX";
    assert(mkdtemp(scratch));
    char cwd[1024];
    assert(getcwd(cwd, sizeof(cwd)) && chdir(scratch) == 0);
    write_fixture("source", "payload\n", 8);
    CHECK("cat", "", "payload$\n", 0, "-E", "source");
    write_fixture("empty", "", 0);
    static const char checksums[] = PAYLOAD_SHA256 "  source\n" EMPTY_SHA256 "  empty\n";
    static const char mismatch[] = EMPTY_SHA256 "  source\n";
    write_fixture("SHA256SUMS", checksums, sizeof(checksums) - 1);
    write_fixture("wrong", mismatch, sizeof(mismatch) - 1);
    CHECK("sha256sum", "", PAYLOAD_SHA256 "  source\n", 0, "source");
    CHECK("sha256sum", "", checksums, 0, "source", "empty");
    CHECK_FILE_INPUT("sha256sum", "source", PAYLOAD_SHA256 "  -\n", 0, "-");
    CHECK("sha256sum", "", "source: OK\nempty: OK\n", 0, "-c", "SHA256SUMS");
    CHECK_FILE_INPUT("sha256sum", "SHA256SUMS", "source: OK\nempty: OK\n", 0, "-c", "-");
    CHECK("sha256sum", "", "source: FAILED\n", 1, "-c", "wrong");
    CHECK("sha256sum", "", "", 1, "missing");
    assert(unlink("empty") == 0);
    assert(unlink("SHA256SUMS") == 0);
    assert(unlink("wrong") == 0);
    write_fixture("lines", "c\nb\na\na\n", 8);
    CHECK("sort", "", "a\na\nb\nc\n", 0, "lines");
    CHECK("uniq", "", "", 0, "lines", "unique");
    CHECK("cat", "", "c\nb\na\n", 0, "unique");
    assert(unlink("lines") == 0);
    assert(unlink("unique") == 0);
    CHECK("cp", "", "", 0, "--", "source", "copy");
    CHECK("cat", "", "payload\n", 0, "copy");
    CHECK("mv", "", "", 0, "--", "copy", "renamed");
    CHECK("stat", "", "8\n", 0, "-c", "%s", "renamed");
    CHECK("mkdir", "", "", 0, "tree");
    CHECK("mkdir", "", "", 0, "-p", "tree/nested/leaf");
    CHECK("cp", "", "", 0, "-t", "tree/nested", "--", "source", "renamed");
    CHECK("cat", "", "payload\n", 0, "tree/nested/source");
    CHECK("cat", "", "payload\n", 0, "tree/nested/renamed");
    CHECK("cp", "", "", 0, "--", "source", "tree/file");
    CHECK("cp", "", "", 0, "-R", "--", "tree", "tree-copy");
    CHECK("cat", "", "payload\n", 0, "tree-copy/file");
    CHECK("rm", "", "", 0, "-r", "--", "tree", "tree-copy");
    CHECK("rm", "", "", 0, "--", "source", "renamed");
    assert(chdir(cwd) == 0);
    assert(rmdir(scratch) == 0);
    printf("PASS coreutils commands checks=%u\n", checks);
    return 0;
}
