#ifndef LUME_USER_LIB_H
#define LUME_USER_LIB_H
#include "lume/abi.h"
#include <stddef.h>
#ifdef LUME_USER_HOST_TEST
int64_t call(uint64_t, uint64_t, uint64_t, uint64_t);
#else
#include "lume/syscall_native.h"
#define call lume_native_call

#endif
static size_t len(const char *s)
{
    size_t n = 0;
    while (s[n])
        ++n;
    return n;
}

static int eq(const char *a, const char *b)
{
    while (*a && *a == *b)
    {
        ++a;
        ++b;
    }
    return *a == *b;
}

static int64_t open(const char *p, uint64_t flags)
{
    return call(LUME_SYS_OPEN, (uintptr_t)p, flags, 0);
}

static int64_t close(int fd)
{
    return call(LUME_SYS_CLOSE, (uint64_t)fd, 0, 0);
}

static int64_t read(int fd, void *p, size_t n)
{
    return call(LUME_SYS_READ, (uint64_t)fd, (uintptr_t)p, n);
}

static int64_t write(int fd, const void *p, size_t n)
{
    return call(LUME_SYS_WRITE, (uint64_t)fd, (uintptr_t)p, n);
}

static int64_t write_all(int fd, const void *data, size_t bytes)
{
#if SIZE_MAX > INT64_MAX
    if (bytes > (uint64_t)INT64_MAX)
        return -75;
#endif
    size_t done = 0;
    while (done < bytes)
    {
        int64_t written = write(fd, (const char *)data + done, bytes - done);
        if (written == -4)
            continue;
        if (written <= 0)
            return done ? (int64_t)done : written;
        if ((uint64_t)written > bytes - done)
            return done ? (int64_t)done : -5;
        done += (size_t)written;
    }
    return (int64_t)done;
}

static void puts(const char *s)
{
    (void)write_all(1, s, len(s));
}
#endif
