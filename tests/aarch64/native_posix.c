#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <spawn.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    const char *path = "/data/posix-check.dat";
    int fd = open(path, O_CREAT | O_TRUNC | O_RDWR, 0600);
    assert(fd >= 3);
    assert(write(fd, "directory-and-spawn", 19) == 19);
    assert(fsync(fd) == 0);
    struct stat by_name, by_fd;
    assert(stat(path, &by_name) == 0 && fstat(fd, &by_fd) == 0);
    assert(by_name.st_ino && by_name.st_ino == by_fd.st_ino);
    assert(by_name.st_size == 19 && S_ISREG(by_name.st_mode));
    assert(lseek(fd, 11, SEEK_SET) == 11);
    assert(ftruncate(fd, 7) == 0);
    assert(lseek(fd, 0, SEEK_CUR) == 11);
    assert(fstat(fd, &by_fd) == 0 && by_fd.st_size == 7);
    errno = 0;
    assert(ftruncate(fd, 20) == -1 && errno == ENOTSUP);
    assert(close(fd) == 0);

    DIR *directory = opendir("/data");
    assert(directory);
    unsigned found = 0;
    struct dirent *entry;
    errno = 0;
    while ((entry = readdir(directory)))
    {
        if (!strcmp(entry->d_name, "posix-check.dat"))
        {
            assert(entry->d_ino == by_name.st_ino && entry->d_type == DT_REG);
            ++found;
        }
    }
    assert(!errno && found == 1);
    rewinddir(directory);
    assert(readdir(directory));
    long position = telldir(directory);
    assert(position > 0);
    seekdir(directory, 0);
    assert(telldir(directory) == 0 && readdir(directory));
    assert(closedir(directory) == 0);

    assert(access(path, R_OK | W_OK) == 0);
    errno = 0;
    assert(access("/data/no-such-posix-file", F_OK) == -1 && errno == ENOENT);
    assert(chdir("/data") == 0);
    char cwd[128];
    assert(getcwd(cwd, sizeof(cwd)) && !strcmp(cwd, "/data"));
    char *const environment[] = {"PATH=/bin:/usr/bin", "HOME=/data/home", NULL};
    char *const arguments[] = {"false", NULL};
    pid_t child;
    assert(posix_spawnp(&child, "false", NULL, NULL, arguments, environment) == 0);
    int status = 0;
    pid_t observed = waitpid(child, &status, WNOHANG);
    assert(observed == 0 || observed == child);
    if (!observed)
        assert(waitpid(child, &status, 0) == child);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 1);
    errno = 0;
    assert(waitpid(child, &status, WNOHANG) == -1 && errno == ECHILD);
    assert(posix_spawnp(&child, "missing-posix-executable", NULL, NULL, arguments, environment) ==
           ENOENT);
    assert(unlink(path) == 0);
    puts("NATIVE_POSIX_PASS inode/directory/truncate/cwd/spawn/wait");
    return 0;
}
