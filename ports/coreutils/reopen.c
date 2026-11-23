/* SPDX-License-Identifier: BSD-3-Clause */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#undef freopen

/* The standard streams keep their runtime callbacks when their descriptors change. */
static FILE *reopen_standard(const char *filename, int flags, int target, FILE *stream)
{
    if (target != STDIN_FILENO)
    {
        fflush(stream);
    }
    int fd = open(filename, flags, 0666);
    int result = fd < 0 ? -1 : dup2(fd, target);
    int saved_errno = errno;
    if (fd >= 0 && fd != target)
    {
        close(fd);
    }
    if (result < 0)
    {
        close(target);
        errno = saved_errno;
        return NULL;
    }
    clearerr(stream);
    stream->unget = 0;
    return stream;
}

FILE *lume_coreutils_freopen(const char *filename, const char *mode, FILE *stream)
{
    if (filename && mode[0] && (!mode[1] || ((mode[1] == 'b' || mode[1] == 't') && !mode[2])))
    {
        if (stream == stdin && mode[0] == 'r')
        {
            return reopen_standard(filename, O_RDONLY, STDIN_FILENO, stream);
        }
        if ((stream == stdout || stream == stderr) && (mode[0] == 'w' || mode[0] == 'a'))
        {
            int flags = O_WRONLY | O_CREAT | (mode[0] == 'w' ? O_TRUNC : O_APPEND);
            int target = stream == stdout ? STDOUT_FILENO : STDERR_FILENO;
            return reopen_standard(filename, flags, target, stream);
        }
    }
    return freopen(filename, mode, stream);
}
