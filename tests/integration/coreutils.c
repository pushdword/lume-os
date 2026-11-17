/* SPDX-License-Identifier: BSD-3-Clause */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <spawn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;
static const char *binary_directory;
static unsigned checks;

static void run(const char *name, const char *const arguments[], const void *input,
                size_t input_bytes, const void *expected, size_t expected_bytes, int status)
{
    char path[1024];
    int length = snprintf(path, sizeof(path), "%s/%s", binary_directory, name);
    assert(length > 0 && (size_t)length < sizeof(path));
    int in[2], out[2];
    assert(pipe(in) == 0);
    assert(pipe(out) == 0);
    posix_spawn_file_actions_t actions;
    assert(posix_spawn_file_actions_init(&actions) == 0);
    assert(posix_spawn_file_actions_adddup2(&actions, in[0], 0) == 0);
    assert(posix_spawn_file_actions_adddup2(&actions, out[1], 1) == 0);
    assert(posix_spawn_file_actions_addclose(&actions, in[0]) == 0);
    assert(posix_spawn_file_actions_addclose(&actions, in[1]) == 0);
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
    assert(close(in[1]) == 0);
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

#define CHECK(name, input, output, status, ...)                                                    \
    run(name, (const char *const[]){name, __VA_ARGS__, NULL}, input, sizeof(input) - 1, output,    \
        sizeof(output) - 1, status)

int main(int argc, char **argv)
{
    if (argc != 2 || argv[1][0] != '/')
    {
        fprintf(stderr, "Usage: %s /absolute/path/to/coreutils/bin\n", argv[0]);
        return 2;
    }
    binary_directory = argv[1];
    CHECK("cat", "a\0b\n", "a\0b\n", 0, "-");
    CHECK("head", "abcdef", "abc", 0, "-c", "3");
    CHECK("tail", "abcdef", "def", 0, "-c", "3");
    CHECK("sort", "10\n2\n1\n", "1\n2\n10\n", 0, "-n");
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
    CHECK("test", "", "", 0, "42", "-eq", "42");
    CHECK("test", "", "", 1, "42", "-eq", "43");
    CHECK("[", "", "", 0, "x", "=", "x", "]");
    CHECK("true", "", "", 0, "--");
    CHECK("false", "", "", 1, "--");

    char scratch[] = "/tmp/coreutils-XXXXXX";
    assert(mkdtemp(scratch));
    char cwd[1024];
    assert(getcwd(cwd, sizeof(cwd)) && chdir(scratch) == 0);
    int fd = open("source", O_WRONLY | O_CREAT | O_EXCL, 0600);
    assert(fd >= 0);
    assert(write(fd, "payload\n", 8) == 8);
    assert(close(fd) == 0);
    CHECK("cp", "", "", 0, "--", "source", "copy");
    CHECK("cat", "", "payload\n", 0, "copy");
    CHECK("mv", "", "", 0, "--", "copy", "renamed");
    CHECK("stat", "", "8\n", 0, "-c", "%s", "renamed");
    CHECK("mkdir", "", "", 0, "tree");
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
