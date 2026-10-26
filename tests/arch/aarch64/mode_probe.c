#include "lume/abi.h"
#include "lume/credential.h"
#include "lume/file_mode.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int64_t native(unsigned number, uint64_t a, uint64_t b, uint64_t c)
{
    register uint64_t x8 __asm__("x8") = number;
    register uint64_t x0 __asm__("x0") = a, x1 __asm__("x1") = b, x2 __asm__("x2") = c;
    __asm__ volatile("svc #0x4c55" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2) : "memory", "cc");
    return (int64_t)x0;
}

static void require(int value, const char *message)
{
    if (!value)
    {
        fprintf(stderr, "MODE_PROBE_FAIL %s\n", message);
        exit(1);
    }
}

int main(int argc, char **argv)
{
    require(argc == 2, "arguments");
    static const char path[] = "/data/mode-probe/host-key";
    static const char temporary[] = "/data/mode-probe/new-key";
    static const char contents[] = "HOST_KEY_PERSISTENCE";
    if (!strcmp(argv[1], "prepare"))
    {
        struct lume_credential_request mask = {
            .version = LUME_CREDENTIAL_VERSION, .bytes = sizeof(mask), .uid = 077};
        require(native(LUME_SYS_CREDENTIAL, LUME_CREDENTIAL_UMASK, (uintptr_t)&mask,
                       sizeof(mask)) == 022,
                "initial umask");
        require(!mkdir("/data/mode-probe", 0777), "directory creation mode");
        int fd = open(temporary, O_RDWR | O_CREAT | O_EXCL, 0666);
        require(fd >= 0, "private file creation");
        require(write(fd, contents, sizeof(contents)) == sizeof(contents), "private file contents");
        struct stat metadata;
        require(!fstat(fd, &metadata) && (metadata.st_mode & 0777) == 0600,
                "umask applies to creation");
        struct lume_file_mode_request mode = {
            .version = LUME_FILE_MODE_VERSION, .bytes = sizeof(mode), .fd = fd, .mode = 0640};
        require(!native(LUME_SYS_FILE_MODE, LUME_FILE_MODE_FD, (uintptr_t)&mode, sizeof(mode)),
                "descriptor chmod");
        require(!fstat(fd, &metadata) && (metadata.st_mode & 0777) == 0640,
                "descriptor mode observed");
        require(!fsync(fd) && !close(fd) && !rename(temporary, path), "private file rename");
        mode.fd = 0;
        mode.path = (uintptr_t)path;
        mode.mode = 0600;
        require(!native(LUME_SYS_FILE_MODE, LUME_FILE_MODE_PATH, (uintptr_t)&mode, sizeof(mode)),
                "path chmod");
        puts("MODE_PREPARE_PASS");
    }
    else
        require(!strcmp(argv[1], "check"), "operation");
    struct stat metadata;
    require(!stat(path, &metadata) && (metadata.st_mode & 0777) == 0600,
            "private mode persistence");
    require(!stat("/data/mode-probe", &metadata) && S_ISDIR(metadata.st_mode) &&
                (metadata.st_mode & 0777) == 0700,
            "directory mode persistence");
    int fd = open(path, O_RDONLY);
    char actual[sizeof(contents)];
    require(fd >= 0 && read(fd, actual, sizeof(actual)) == sizeof(actual) &&
                !memcmp(actual, contents, sizeof(actual)),
            "private data persistence");
    require(!close(fd), "private file close");
    puts("MODE_PERSISTENCE_PASS");
    return 0;
}
