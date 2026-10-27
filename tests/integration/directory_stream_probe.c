#define _DEFAULT_SOURCE 1
#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fd.h>
#include "lume/abi.h"
#include "lume/credential.h"
#include "lume/syscall_native.h"

#define FILE_COUNT 35u

static int descriptor_count(void)
{
    struct lume_fd_info info;
    if ((uintptr_t)&info > UINT32_MAX)
        return -1;
    struct lume_fd_request request = {
        .version = LUME_FD_VERSION, .bytes = sizeof(request), .buffer = (uint32_t)(uintptr_t)&info};
    if (lume_native_call(LUME_SYS_FD, LUME_FD_INFO, (uintptr_t)&request, sizeof(request)))
        return -1;
    return (int)info.open;
}

static bool rejected_directory(const char *path, int expected)
{
    int before = descriptor_count();
    errno = 0;
    DIR *directory = opendir(path);
    int observed = errno;
    if (directory)
        (void)closedir(directory);
    return !directory && observed == expected && before >= 0 && descriptor_count() == before;
}

static bool same_entry(const struct dirent *left, const struct dirent *right)
{
    return left->d_ino == right->d_ino && left->d_type == right->d_type &&
           !strcmp(left->d_name, right->d_name);
}

static int jailed_directory_child(const char *empty, const char *outside)
{
    int before = descriptor_count();
    if (before < 0)
        return 10;
    struct lume_credential_request request = {
        .version = LUME_CREDENTIAL_VERSION, .bytes = sizeof(request), .path = (uintptr_t)empty};
    if (lume_native_call(LUME_SYS_CREDENTIAL, LUME_CREDENTIAL_CHROOT, (uintptr_t)&request,
                         sizeof(request)))
        return 11;
    for (unsigned pass = 0; pass < 2; ++pass)
    {
        if (pass)
        {
            request = (struct lume_credential_request){.version = LUME_CREDENTIAL_VERSION,
                                                       .bytes = sizeof(request),
                                                       .uid = 65534,
                                                       .euid = 65534,
                                                       .suid = 65534};
            if (lume_native_call(LUME_SYS_CREDENTIAL, LUME_CREDENTIAL_UID, (uintptr_t)&request,
                                 sizeof(request)))
                return 21;
        }
        DIR *directory = opendir("/");
        if (!directory)
            return 12 + (int)pass * 20;
        int flags = fcntl(directory->fd, F_GETFD);
        if (flags < 0 || !(flags & FD_CLOEXEC))
            return 13 + (int)pass * 20;
        errno = 0;
        if (readdir(directory) || errno)
            return 14 + (int)pass * 20;
        if (closedir(directory))
            return 15 + (int)pass * 20;
        if (descriptor_count() != before)
            return 16 + (int)pass * 20;
        errno = 0;
        directory = opendir(outside);
        int error = errno;
        if (directory)
            (void)closedir(directory);
        if (directory || error != ENOENT)
            return 17 + (int)pass * 20;
        if (descriptor_count() != before)
            return 18 + (int)pass * 20;
    }
    return 0;
}

static bool jailed_directory(const char *empty, const char *outside)
{
    char before_cwd[128], after_cwd[128];
    struct stat metadata;
    int before = descriptor_count();
    if (before < 0 || !getcwd(before_cwd, sizeof(before_cwd)) || fflush(stdout) || fflush(stderr))
        return false;
    int64_t child = lume_native_call(LUME_SYS_FORK, 0, 0, 0);
    if (child < 0)
        return false;
    if (!child)
        _exit(jailed_directory_child(empty, outside));
    int status = 0;
    pid_t waited;
    do
    {
        waited = waitpid((pid_t)child, &status, 0);
    } while (waited < 0 && errno == EINTR);
    if (waited != child || !WIFEXITED(status) || WEXITSTATUS(status))
    {
        fprintf(stderr, "DIRECTORY_JAIL_FAIL child=%ld status=%d\n", (long)child, status);
        return false;
    }
    if (descriptor_count() != before || !getcwd(after_cwd, sizeof(after_cwd)) ||
        strcmp(before_cwd, after_cwd) || stat(outside, &metadata) || !S_ISREG(metadata.st_mode))
        return false;
    puts("DIRECTORY_JAIL_PASS root/unprivileged/EOF/CLOEXEC/noescape/FD cleanup");
    return true;
}

#define REQUIRE(condition)                                                                         \
    do                                                                                             \
    {                                                                                              \
        if (!(condition))                                                                          \
        {                                                                                          \
            fprintf(stderr, "DIRECTORY_STREAM_FAIL line=%u errno=%d\n", __LINE__, errno);          \
            goto cleanup;                                                                          \
        }                                                                                          \
    } while (0)

int main(void)
{
    char saved_cwd[128] = {0}, directory_path[128] = {0}, path[128];
    DIR *directory = NULL;
    bool created_directory = false, created_nested = false;
    unsigned created_files = 0;
    int fd = -1, result = 1;
    int before = descriptor_count();
    REQUIRE(before >= 0);
    REQUIRE(getcwd(saved_cwd, sizeof(saved_cwd)));
    REQUIRE(snprintf(directory_path, sizeof(directory_path), "/tmp/directory-stream-%ld",
                     (long)getpid()) > 0);
    REQUIRE(!mkdir(directory_path, 0700));
    created_directory = true;
    errno = 0;
    REQUIRE(mkdir(directory_path, 0700) == -1 && errno == EEXIST);
    for (unsigned i = 0; i < FILE_COUNT; ++i)
    {
        REQUIRE(snprintf(path, sizeof(path), "%s/file-%02u", directory_path, i) > 0);
        fd = open(path, O_CREAT | O_EXCL | O_WRONLY, 0600);
        REQUIRE(fd >= 0);
        ++created_files;
        REQUIRE(!close(fd));
        fd = -1;
    }
    REQUIRE(snprintf(path, sizeof(path), "%s/nested", directory_path) > 0);
    REQUIRE(!mkdir(path, 0700));
    created_nested = true;
    REQUIRE(snprintf(path, sizeof(path), "%s/file-00", directory_path) > 0);
    REQUIRE(rejected_directory(path, ENOTDIR));
    errno = 0;
    REQUIRE(mkdir(path, 0700) == -1 && errno == EEXIST);
    REQUIRE(snprintf(path, sizeof(path), "%s/missing", directory_path) > 0);
    REQUIRE(rejected_directory(path, ENOENT));
    REQUIRE(descriptor_count() == before);

    REQUIRE(!chdir(directory_path));
    directory = opendir(".");
    REQUIRE(directory && descriptor_count() == before + 1);
    int descriptor_flags = fcntl(directory->fd, F_GETFD);
    REQUIRE(descriptor_flags >= 0 && (descriptor_flags & FD_CLOEXEC));
    struct dirent *entry = readdir(directory);
    REQUIRE(entry);
    struct dirent first = *entry;
    long first_cookie = telldir(directory);
    REQUIRE(first_cookie > 0 && directory->next == 1 && directory->count > 1);
    REQUIRE(lseek(directory->fd, 0, SEEK_CUR) > first_cookie);
    entry = readdir(directory);
    REQUIRE(entry);
    struct dirent second = *entry;
    long second_cookie = telldir(directory);
    REQUIRE(second_cookie > first_cookie);
    seekdir(directory, first_cookie);
    REQUIRE(telldir(directory) == first_cookie);
    entry = readdir(directory);
    REQUIRE(entry && same_entry(entry, &second) && telldir(directory) == second_cookie);
    rewinddir(directory);
    REQUIRE(telldir(directory) == 0);
    entry = readdir(directory);
    REQUIRE(entry && same_entry(entry, &first));
    long unchanged = telldir(directory);
    errno = 0;
    seekdir(directory, -1);
    REQUIRE(errno == EINVAL && telldir(directory) == unchanged);

    rewinddir(directory);
    REQUIRE(!chdir("/"));
    uint64_t seen = 0;
    unsigned entries = 0;
    errno = 0;
    while ((entry = readdir(directory)))
    {
        unsigned index = FILE_COUNT;
        if (strcmp(entry->d_name, "nested"))
        {
            REQUIRE(strlen(entry->d_name) == 7 && !strncmp(entry->d_name, "file-", 5));
            REQUIRE(entry->d_name[5] >= '0' && entry->d_name[5] <= '9' && entry->d_name[6] >= '0' &&
                    entry->d_name[6] <= '9');
            index = (unsigned)(entry->d_name[5] - '0') * 10u + (unsigned)(entry->d_name[6] - '0');
            REQUIRE(index < FILE_COUNT && entry->d_type == DT_REG);
        }
        else
            REQUIRE(entry->d_type == DT_DIR);
        REQUIRE(entry->d_ino && !(seen & (UINT64_C(1) << index)));
        seen |= UINT64_C(1) << index;
        ++entries;
    }
    REQUIRE(!errno && entries == FILE_COUNT + 1 && seen == (UINT64_C(1) << (FILE_COUNT + 1)) - 1);
    errno = EDOM;
    REQUIRE(!readdir(directory) && errno == EDOM);
    seekdir(directory, first_cookie);
    entry = readdir(directory);
    REQUIRE(entry && same_entry(entry, &second));
    rewinddir(directory);
    entry = readdir(directory);
    REQUIRE(entry && same_entry(entry, &first));
    int directory_fd = directory->fd;
    int close_result = closedir(directory);
    directory = NULL;
    REQUIRE(!close_result);
    errno = 0;
    REQUIRE(fcntl(directory_fd, F_GETFD) == -1 && errno == EBADF);
    REQUIRE(descriptor_count() == before);
    REQUIRE(snprintf(path, sizeof(path), "%s/nested", directory_path) > 0);
    char outside_path[128];
    REQUIRE(snprintf(outside_path, sizeof(outside_path), "%s/file-00", directory_path) > 0);
    REQUIRE(jailed_directory(path, outside_path));
    result = 0;

cleanup:
    if (fd >= 0 && close(fd))
        result = 1;
    if (directory && closedir(directory))
        result = 1;
    if (saved_cwd[0] && chdir(saved_cwd))
        result = 1;
    for (unsigned i = 0; i < created_files; ++i)
    {
        (void)snprintf(path, sizeof(path), "%s/file-%02u", directory_path, i);
        if (unlink(path))
            result = 1;
    }
    if (created_nested)
    {
        (void)snprintf(path, sizeof(path), "%s/nested", directory_path);
        if (rmdir(path))
            result = 1;
    }
    if (created_directory && rmdir(directory_path))
        result = 1;
    if (descriptor_count() != before)
        result = 1;
    if (!result)
        puts("DIRECTORY_STREAM_PASS batch/cookies/rewind/EOF/cwd/CLOEXEC/errors/FD cleanup");
    return result;
}
