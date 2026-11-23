/* SPDX-License-Identifier: BSD-3-Clause */
#include "auth_files.h"
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

static int source_binary(void)
{
    int usr = auth_trusted_directory(AT_FDCWD, "/usr", false, 0755);
    if (usr < 0)
        return -1;
    int libexec = auth_trusted_directory(usr, "libexec", false, 0755);
    close(usr);
    if (libexec < 0)
        return -1;
    int file = openat(libexec, "sudo", O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC);
    close(libexec);
    if (file < 0)
        return -1;
    struct stat info;
    if (!fstat(file, &info) && S_ISREG(info.st_mode) && !info.st_uid && !(info.st_mode & 022u) &&
        info.st_size > 0 && info.st_size <= 4 * 1024 * 1024)
        return file;
    close(file);
    errno = EACCES;
    return -1;
}

static int copy_binary(int source, int destination)
{
    char buffer[8192];
    for (;;)
    {
        ssize_t bytes = read(source, buffer, sizeof(buffer));
        if (!bytes)
            return 0;
        if (bytes < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        for (ssize_t offset = 0; offset < bytes;)
        {
            ssize_t sent = write(destination, buffer + offset, (size_t)(bytes - offset));
            if (sent > 0)
                offset += sent;
            else if (!sent || errno != EINTR)
            {
                if (!sent)
                    errno = EIO;
                return -1;
            }
        }
    }
}

static int install_binary(int source, int staging, int bin)
{
    uint64_t suffix;
    if (!auth_random((unsigned char *)&suffix, sizeof(suffix)))
        return -1;
    char name[32];
    snprintf(name, sizeof(name), "sudo-%016" PRIx64, suffix);
    int file = openat(staging, name, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600u);
    if (file < 0)
        return -1;
    int saved = 0;
    if (copy_binary(source, file) || fsync(file) || fchmod(file, 04755) || fsync(file))
        saved = errno;
    if (close(file) && !saved)
        saved = errno;
    if (!saved && renameat(staging, name, bin, "sudo"))
        saved = errno;
    if (!saved)
        return fsync(bin);
    (void)unlinkat(staging, name, 0);
    errno = saved;
    return -1;
}

int auth_install_sudo(void)
{
    if (getuid() || geteuid())
    {
        errno = EPERM;
        return -1;
    }
    int source = source_binary();
    if (source < 0)
        return -1;
    int data = auth_trusted_directory(AT_FDCWD, "/data", false, 0755);
    int bin = -1, staging = -1, result = -1;
    if (data >= 0 && auth_permissions_volume(data, true))
    {
        bin = auth_trusted_directory(data, "bin", true, 0755);
        if (bin >= 0)
        {
            staging = auth_trusted_directory(bin, ".sudo-install", true, 0700);
            if (staging >= 0)
                result = install_binary(source, staging, bin);
        }
    }
    int saved = errno;
    if (staging >= 0)
        close(staging);
    if (bin >= 0)
        close(bin);
    if (data >= 0)
        close(data);
    close(source);
    errno = saved;
    return result;
}
