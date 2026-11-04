/* SPDX-License-Identifier: BSD-3-Clause */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>

ssize_t getdelim(char **restrict line, size_t *restrict capacity, int delimiter,
                 FILE *restrict file)
{
    if (!line || !capacity || !file)
    {
        errno = EINVAL;
        return -1;
    }
    flockfile(file);
    size_t available = *line ? *capacity : 0;
    size_t used = 0;
    ssize_t result = -1;
    for (;;)
    {
        int value = getc_unlocked(file);
        if (value == EOF)
        {
            if (used && !ferror(file))
                result = (ssize_t)used;
            break;
        }
        if (used >= (size_t)SSIZE_MAX)
        {
            errno = EOVERFLOW;
            break;
        }
        if (used + 1 >= available)
        {
            size_t next =
                available <= (size_t)SSIZE_MAX / 2 ? available * 2 : (size_t)SSIZE_MAX + 1;
            if (next < 128)
                next = 128;
            char *buffer = realloc(*line, next);
            if (!buffer)
                break;
            *line = buffer;
            *capacity = available = next;
        }
        (*line)[used++] = (char)value;
        (*line)[used] = 0;
        if ((unsigned char)value == (unsigned char)delimiter)
        {
            result = (ssize_t)used;
            break;
        }
    }
    funlockfile(file);
    return result;
}

ssize_t getline(char **restrict line, size_t *restrict capacity, FILE *restrict file)
{
    return getdelim(line, capacity, '\n', file);
}
