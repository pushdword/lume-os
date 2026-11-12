/* SPDX-License-Identifier: BSD-3-Clause */
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct dirent records[35];
static unsigned position, count;
static int read_error, close_error;

static DIR *open_directory(const char *path)
{
    position = 0;
    if (!path)
    {
        errno = ENOENT;
        return NULL;
    }
    return (DIR *)records;
}

static struct dirent *read_directory(DIR *directory)
{
    assert(directory == (DIR *)records);
    if (position < count)
        return records + position++;
    errno = read_error;
    return NULL;
}

static int close_directory(DIR *directory)
{
    assert(directory == (DIR *)records);
    errno = close_error;
    return close_error ? -1 : 0;
}

#define opendir open_directory
#define readdir read_directory
#define closedir close_directory
#define alphasort native_alphasort
#define scandir native_scandir
#include "../../libc/posix/directory_sort.c"

static int select_even(const struct dirent *entry)
{
    return !(atoi(entry->d_name) & 1);
}

static void release(struct dirent **entries, int size)
{
    while (size)
        free(entries[--size]);
    free(entries);
}

int main(void)
{
    for (unsigned i = 0; i < 35; ++i)
        snprintf(records[i].d_name, sizeof(records[i].d_name), "%02u", 34 - i);
    for (count = 0; count <= 35; ++count)
    {
        struct dirent **entries = NULL;
        int result = native_scandir("/", &entries, NULL, native_alphasort);
        assert(result == (int)count);
        for (int i = 1; i < result; ++i)
            assert(strcmp(entries[i - 1]->d_name, entries[i]->d_name) < 0);
        release(entries, result);
    }
    count = 35;
    struct dirent **entries = NULL;
    int result = native_scandir("/", &entries, select_even, native_alphasort);
    assert(result == 18);
    for (int i = 0; i < result; ++i)
        assert(atoi(entries[i]->d_name) == i * 2);
    release(entries, result);
    entries = NULL;
    read_error = EIO;
    assert(native_scandir("/", &entries, NULL, NULL) == -1 && errno == EIO && !entries);
    read_error = 0;
    close_error = EIO;
    assert(native_scandir("/", &entries, NULL, NULL) == -1 && errno == EIO && !entries);
    assert(native_scandir(NULL, &entries, NULL, NULL) == -1 && errno == ENOENT);
    return 0;
}
