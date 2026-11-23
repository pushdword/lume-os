#define LUME_STAT_IMPLEMENTATION 1
#include "api.h"
#include "errno_map.h"
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <sys/time.h>
#include <fcntl.h>
#include <utime.h>
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
#include "file_owner_client.h"
#endif

int creat(const char *path, mode_t mode)
{
    return open(path, O_WRONLY | O_CREAT | O_TRUNC, mode);
}

static int change_file_times(int directory, const char *path, const struct timespec times[2],
                             int flags, int use_fd)
{
    if (!use_fd && !path)
    {
        errno = EFAULT;
        return -1;
    }
    if (flags & ~AT_SYMLINK_NOFOLLOW)
    {
        errno = EINVAL;
        return -1;
    }
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
    struct lume_file_time values[2] = {
        {.nanoseconds = LUME_FILE_TIME_NOW},
        {.nanoseconds = LUME_FILE_TIME_NOW},
    };
    for (unsigned i = 0; times && i < 2; ++i)
    {
        long nanoseconds = times[i].tv_nsec;
        if (nanoseconds == UTIME_OMIT)
            values[i].nanoseconds = LUME_FILE_TIME_OMIT;
        else if (nanoseconds != UTIME_NOW)
        {
            if (nanoseconds < 0 || nanoseconds >= 1000000000)
            {
                errno = EINVAL;
                return -1;
            }
            values[i] = (struct lume_file_time){.seconds = times[i].tv_sec,
                                                .nanoseconds = (uint32_t)nanoseconds};
        }
    }
    struct lume_files_request request = {
        .path = (uintptr_t)path,
        .fd = !use_fd && directory == AT_FDCWD ? LUME_FILES_AT_CWD : directory,
        .flags = use_fd                     ? LUME_FILES_TIMES_FD
                 : flags & AT_SYMLINK_NOFOLLOW ? LUME_FILES_TIMES_NOFOLLOW
                                               : 0,
        .buffer = (uintptr_t)values,
        .count = sizeof(values),
    };
    return lume_files_result(lume_native_files_call(LUME_FILES_TIMES, &request));
#else
    (void)times;
    if (use_fd && fcntl(directory, F_GETFD) < 0)
        return -1;
    errno = ENOTSUP;
    return -1;
#endif
}

int futimens(int fd, const struct timespec times[2])
{
    return change_file_times(fd, NULL, times, 0, 1);
}

int utimensat(int directory, const char *path, const struct timespec times[2], int flags)
{
    return change_file_times(directory, path, times, flags, 0);
}

static int change_timeval(int fd, const char *path, const struct timeval times[2], int flags,
                          int use_fd)
{
    struct timespec values[2];
    for (unsigned i = 0; times && i < 2; ++i)
    {
        if (times[i].tv_usec < 0 || times[i].tv_usec >= 1000000)
        {
            errno = EINVAL;
            return -1;
        }
        values[i] = (struct timespec){.tv_sec = times[i].tv_sec,
                                      .tv_nsec = times[i].tv_usec * 1000L};
    }
    return change_file_times(fd, path, times ? values : NULL, flags, use_fd);
}

int futimes(int fd, const struct timeval times[2])
{
    return change_timeval(fd, NULL, times, 0, 1);
}

int lutimes(const char *path, const struct timeval times[2])
{
    return change_timeval(AT_FDCWD, path, times, AT_SYMLINK_NOFOLLOW, 0);
}

int utime(const char *path, const struct utimbuf *times)
{
    if (!times)
        return utimensat(AT_FDCWD, path, NULL, 0);
    const struct timespec values[2] = {{.tv_sec = times->actime}, {.tv_sec = times->modtime}};
    return utimensat(AT_FDCWD, path, values, 0);
}

int lchown(const char *path, uid_t owner, gid_t group)
{
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
    return lume_files_change_owner(LUME_FILE_MODE_OWNER_LINK, path, -1, owner, group);
#else
    (void)owner;
    (void)group;
    if (!path)
    {
        errno = EFAULT;
        return -1;
    }
    if (!*path)
    {
        errno = ENOENT;
        return -1;
    }
    errno = ENOTSUP;
    return -1;
#endif
}

int renameat(int source_directory, const char *source, int target_directory, const char *target)
{
    if (!source || !target)
    {
        errno = EFAULT;
        return -1;
    }
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
    struct lume_files_request request = {
        .path = (uintptr_t)source,
        .path2 = (uintptr_t)target,
        .fd = source_directory == AT_FDCWD ? LUME_FILES_AT_CWD : source_directory,
        .fd2 = target_directory == AT_FDCWD ? LUME_FILES_AT_CWD : target_directory};
    return lume_files_result(lume_native_files_call(LUME_FILES_REPLACE, &request));
#else
    if ((*source != '/' && source_directory != AT_FDCWD) ||
        (*target != '/' && target_directory != AT_FDCWD))
    {
        errno = ENOTSUP;
        return -1;
    }
    return rename(source, target);
#endif
}

int linkat(int source_directory, const char *source, int target_directory, const char *target,
           int flags)
{
    if (!source || !target)
    {
        errno = EFAULT;
        return -1;
    }
    if (flags & ~AT_SYMLINK_FOLLOW)
    {
        errno = EINVAL;
        return -1;
    }
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
    struct lume_files_request request = {
        .path = (uintptr_t)source,
        .path2 = (uintptr_t)target,
        .fd = source_directory == AT_FDCWD ? LUME_FILES_AT_CWD : source_directory,
        .fd2 = target_directory == AT_FDCWD ? LUME_FILES_AT_CWD : target_directory,
        .flags = flags & AT_SYMLINK_FOLLOW ? LUME_FILES_LINK_FOLLOW : 0,
    };
    return lume_files_result(lume_native_files_call(LUME_FILES_LINK, &request));
#else
    (void)source_directory;
    (void)target_directory;
    errno = ENOTSUP;
    return -1;
#endif
}

int symlinkat(const char *target, int directory, const char *path)
{
    if (!target || !path)
    {
        errno = EFAULT;
        return -1;
    }
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
    struct lume_files_request request = {
        .path = (uintptr_t)path,
        .path2 = (uintptr_t)target,
        .fd = directory == AT_FDCWD ? LUME_FILES_AT_CWD : directory,
    };
    return lume_files_result(lume_native_files_call(LUME_FILES_SYMLINK, &request));
#else
    (void)directory;
    errno = ENOTSUP;
    return -1;
#endif
}

int mkdirat(int directory, const char *path, mode_t mode)
{
    if (!path)
    {
        errno = EFAULT;
        return -1;
    }
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
    struct lume_files_request request = {.path = (uintptr_t)path,
                                         .fd =
                                             directory == AT_FDCWD ? LUME_FILES_AT_CWD : directory,
                                         .mode = mode & 01777u};
    return lume_files_result(lume_native_files_call(LUME_FILES_MKDIR, &request));
#else
    if (*path != '/' && directory != AT_FDCWD)
    {
        errno = ENOTSUP;
        return -1;
    }
    return mkdir(path, mode);
#endif
}

int mkfifoat(int directory, const char *path, mode_t mode)
{
    if (!path)
    {
        errno = EFAULT;
        return -1;
    }
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
    struct lume_files_request request = {
        .path = (uintptr_t)path,
        .fd = directory == AT_FDCWD ? LUME_FILES_AT_CWD : directory,
        .mode = mode & 0777u,
    };
    return lume_files_result(lume_native_files_call(LUME_FILES_MKFIFO, &request));
#else
    (void)directory;
    (void)mode;
    errno = ENOTSUP;
    return -1;
#endif
}

int mkfifo(const char *path, mode_t mode)
{
    return mkfifoat(AT_FDCWD, path, mode);
}

int unlinkat(int directory, const char *path, int flags)
{
    if (!path)
    {
        errno = EFAULT;
        return -1;
    }
    if (flags & ~AT_REMOVEDIR)
    {
        errno = EINVAL;
        return -1;
    }
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
    struct lume_files_request request = {
        .path = (uintptr_t)path, .fd = directory == AT_FDCWD ? LUME_FILES_AT_CWD : directory};
    return lume_files_result(lume_native_files_call(
        flags & AT_REMOVEDIR ? LUME_FILES_RMDIR : LUME_FILES_UNLINK, &request));
#else
    if (*path != '/' && directory != AT_FDCWD)
    {
        errno = ENOTSUP;
        return -1;
    }
    return flags & AT_REMOVEDIR ? rmdir(path) : unlink(path);
#endif
}

int utimes(const char *path, const struct timeval times[2])
{
    return change_timeval(AT_FDCWD, path, times, 0, 0);
}

int remove(const char *path)
{
    int result = unlink(path);
    return result < 0 && errno == EISDIR ? rmdir(path) : result;
}

int lstat(const char *path, struct stat *metadata)
{
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
    return lume_files_result(
        lume_native_stat_at(LUME_FILES_AT_CWD, path, LUME_FILES_STAT_NOFOLLOW, metadata));
#else
    return stat(path, metadata);
#endif
}

int access(const char *path, int mode)
{
#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
    return faccessat(AT_FDCWD, path, mode, 0);
#else
    if (mode & ~(R_OK | W_OK | X_OK))
    {
        errno = EINVAL;
        return -1;
    }
    struct stat metadata;
    if (stat(path, &metadata))
        return -1;
    unsigned permissions = metadata.st_mode;
    if (((mode & R_OK) && !(permissions & 0444)) || ((mode & W_OK) && !(permissions & 0222)) ||
        ((mode & X_OK) && !(permissions & 0111)))
    {
        errno = EACCES;
        return -1;
    }
    return 0;
#endif
}

#if !defined(__aarch64__) && !defined(__x86_64__) && !defined(__i386__)
int ftruncate(int fd, off_t length)
{
    struct stat metadata;
    if (length < 0)
    {
        errno = EINVAL;
        return -1;
    }
    if (fstat(fd, &metadata))
        return -1;
    if (!S_ISREG(metadata.st_mode))
    {
        errno = EINVAL;
        return -1;
    }
    if (length > metadata.st_size)
    {
        errno = ENOTSUP;
        return -1;
    }
    off_t previous = lseek(fd, 0, SEEK_CUR);
    if (previous < 0 || lseek(fd, length, SEEK_SET) < 0)
        return -1;
    int result = lume_syscall(LUME_TRUNCATE_HERE, fd, 0, 0);
    int saved = result < 0 ? lume_errno_from_syscall((unsigned)-result) : 0;
    if (lseek(fd, previous, SEEK_SET) < 0 && !saved)
        return -1;
    if (saved)
    {
        errno = saved;
        return -1;
    }
    return 0;
}
#endif

#if defined(LUME_ARCH_AARCH64) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
int fchmodat(int directory, const char *path, mode_t mode, int flags)
{
    if (!path)
    {
        errno = EFAULT;
        return -1;
    }
    if (flags & ~AT_SYMLINK_NOFOLLOW)
    {
        errno = EINVAL;
        return -1;
    }
    if (mode & ~(mode_t)07777u)
    {
        errno = EINVAL;
        return -1;
    }
    struct lume_files_request request = {
        .path = (uintptr_t)path,
        .fd = directory == AT_FDCWD ? LUME_FILES_AT_CWD : directory,
        .flags = flags & AT_SYMLINK_NOFOLLOW ? LUME_FILES_MODE_NOFOLLOW : 0,
        .mode = mode};
    return lume_files_result(lume_native_files_call(LUME_FILES_MODE, &request));
}

int truncate(const char *path, off_t length)
{
    if (length < 0)
    {
        errno = EINVAL;
        return -1;
    }
    if (!path)
    {
        errno = EFAULT;
        return -1;
    }
    int fd = open(path, O_WRONLY | O_CLOEXEC);
    if (fd < 0)
        return -1;
    int result = ftruncate(fd, length);
    int saved = errno;
    int closed = close(fd);
    if (result < 0)
    {
        errno = saved;
        return -1;
    }
    return closed;
}

int symlink(const char *target, const char *path)
{
    return symlinkat(target, AT_FDCWD, path);
}

#endif
