/* SPDX-License-Identifier: BSD-3-Clause */
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int alphasort(const struct dirent **left, const struct dirent **right)
{
    return strcoll((*left)->d_name, (*right)->d_name);
}

static void sift(struct dirent **entries, size_t root, size_t count,
                 int (*compare)(const struct dirent **, const struct dirent **))
{
    while (root < count / 2)
    {
        size_t child = root * 2 + 1;
        const struct dirent *left = entries[child], *right;
        if (child + 1 < count)
        {
            right = entries[child + 1];
            if (compare(&left, &right) < 0)
                left = entries[++child];
        }
        right = entries[root];
        if (compare(&right, &left) >= 0)
            break;
        struct dirent *entry = entries[root];
        entries[root] = entries[child];
        entries[child] = entry;
        root = child;
    }
}

int scandir(const char *path, struct dirent ***output, int (*select)(const struct dirent *),
            int (*compare)(const struct dirent **, const struct dirent **))
{
    DIR *directory = opendir(path);
    if (!directory)
        return -1;
    struct dirent **entries = NULL;
    size_t count = 0, capacity = 0;
    int error = 0, saved = errno;
    for (;;)
    {
        errno = 0;
        struct dirent *entry = readdir(directory);
        if (!entry)
        {
            error = errno;
            break;
        }
        if (select && !select(entry))
            continue;
        if (count == INT_MAX)
        {
            error = EOVERFLOW;
            break;
        }
        if (count == capacity)
        {
            size_t next = capacity ? capacity * 2 : 16;
            if (next > SIZE_MAX / sizeof(*entries))
            {
                error = ENOMEM;
                break;
            }
            struct dirent **grown = realloc(entries, next * sizeof(*entries));
            if (!grown)
            {
                error = errno;
                break;
            }
            entries = grown;
            capacity = next;
        }
        entries[count] = malloc(sizeof(*entry));
        if (!entries[count])
        {
            error = errno;
            break;
        }
        memcpy(entries[count++], entry, sizeof(*entry));
    }
    if (closedir(directory) && !error)
        error = errno;
    if (error)
    {
        while (count)
            free(entries[--count]);
        free(entries);
        errno = error;
        return -1;
    }
    if (compare && count > 1)
    {
        for (size_t i = count / 2; i; --i)
            sift(entries, i - 1, count, compare);
        for (size_t i = count - 1; i; --i)
        {
            struct dirent *entry = entries[0];
            entries[0] = entries[i];
            entries[i] = entry;
            sift(entries, 0, i, compare);
        }
    }
    *output = entries;
    errno = saved;
    return (int)count;
}
