/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_FILE_OPERATIONS_H
#define LUME_FILE_OPERATIONS_H
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>
#include <unistd.h>

static inline int file_copy(const char *source, const char *destination)
{
    int input = open(source, O_RDONLY);
    if (input < 0)
        return -1;
    struct stat status;
    if (fstat(input, &status) || !S_ISREG(status.st_mode))
    {
        close(input);
        errno = EINVAL;
        return -1;
    }
    int output = open(destination, O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (output < 0)
    {
        int saved = errno;
        close(input);
        errno = saved;
        return -1;
    }
    char buffer[16384];
    int result = 0, saved = 0;
    ssize_t count;
    while ((count = read(input, buffer, sizeof(buffer))) > 0)
    {
        ssize_t done = 0;
        while (done < count)
        {
            ssize_t written = write(output, buffer + done, (size_t)(count - done));
            if (written <= 0)
            {
                saved = written ? errno : EIO;
                result = -1;
                break;
            }
            done += written;
        }
        if (result)
            break;
    }
    if (count < 0 && !result)
    {
        saved = errno;
        result = -1;
    }
    close(input);
    if (close(output) && !result)
    {
        saved = errno;
        result = -1;
    }
    if (result)
    {
        unlink(destination);
        errno = saved;
    }
    return result;
}
#endif
