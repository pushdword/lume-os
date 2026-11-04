/* SPDX-License-Identifier: BSD-3-Clause */
#include <X11/ICE/ICElib.h>
#include <X11/ICE/ICEutil.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    char path[128], lock[128];
    snprintf(path, sizeof(path), "/data/ice-authority-%ld", (long)getpid());
    snprintf(lock, sizeof(lock), "%s-l", path);
    assert(IceLockAuthFile(path, 1, 0, 600) == IceAuthLockSuccess);
    struct stat info;
    assert(!stat(lock, &info) && (info.st_mode & 0777) == 0600);
    pid_t child = fork();
    assert(child >= 0);
    if (!child)
        _exit(IceLockAuthFile(path, 1, 0, 600) == IceAuthLockTimeout ? 0 : 1);
    int status;
    assert(waitpid(child, &status, 0) == child && WIFEXITED(status) && !WEXITSTATUS(status));
    IceUnlockAuthFile(path);
    assert(IceLockAuthFile(path, 1, 0, 600) == IceAuthLockSuccess);
    IceUnlockAuthFile(path);
    assert(stat(lock, &info) < 0);
    char invalid[1024];
    memset(invalid, 'a', sizeof(invalid) - 1);
    invalid[sizeof(invalid) - 1] = 0;
    assert(IceLockAuthFile(invalid, 1, 0, 600) == IceAuthLockError);
    puts("ICE_LOCK_EXCLUSION_RELEASE_REUSE_PASS");
    return 0;
}
