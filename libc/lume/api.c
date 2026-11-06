#include "api.h"

unsigned app_length(const char *s)
{
    unsigned n = 0;
    while (s[n])
        n++;
    return n;
}

int app_equal(const char *a, const char *b)
{
    while (*a && *a == *b)
    {
        a++;
        b++;
    }
    return *a == *b;
}

void app_puts(const char *s)
{
    (void)app_write_all(1, s, app_length(s));
}

void app_number(uint32_t n)
{
    char b[11];
    unsigned i = 10;
    b[i] = 0;
    do
    {
        b[--i] = (char)('0' + n % 10);
        n /= 10;
    } while (n);
    app_puts(b + i);
}

_Noreturn void app_exit(unsigned n)
{
    lume_app_call(LUME_EXIT, n, 0, 0);
    for (;;)
    {
    }
}

static uintptr_t pointer(const void *p)
{
    return (uintptr_t)p;
}

int app_open(const char *p, unsigned f)
{
    return lume_app_call(LUME_OPEN, pointer(p), f, 0);
}

int app_close(int f)
{
    return lume_app_call(LUME_CLOSE, (uint32_t)f, 0, 0);
}

int app_read(int f, void *p, unsigned n)
{
    return lume_app_call(LUME_READ, (uint32_t)f, pointer(p), n);
}

int app_write(int f, const void *p, unsigned n)
{
    return lume_app_call(LUME_FDWRITE, (uint32_t)f, pointer(p), n);
}

int app_write_all(int fd, const void *data, unsigned bytes)
{
    if (bytes > (unsigned)INT32_MAX)
        return -75;
    unsigned done = 0;
    while (done < bytes)
    {
        int written = app_write(fd, (const char *)data + done, bytes - done);
        if (written == -4)
            continue;
        if (written <= 0)
            return done ? (int)done : written;
        if ((unsigned)written > bytes - done)
            return done ? (int)done : -5;
        done += (unsigned)written;
    }
    return (int)done;
}

int app_seek(int f, int32_t off, unsigned whence)
{
    return lume_app_call(LUME_SEEK, (uint32_t)f, (uint32_t)off, whence);
}

int app_sync(int f)
{
    return lume_app_call(LUME_FSYNC, (uint32_t)f, 0, 0);
}

int app_stat(const char *p, struct lume_stat *s)
{
    return lume_app_call(LUME_STAT, pointer(p), pointer(s), 0);
}

int app_spawn(const char *p, unsigned n, const char *const *v)
{
    return lume_app_call(LUME_SPAWN, pointer(p), n, pointer(v));
}

int app_spawn_env(const char *p, unsigned n, const char *const *v, unsigned e,
                  const char *const *env)
{
#if defined(__aarch64__) || defined(LUME_ARCH_AMD64) || defined(LUME_ARCH_I686)
    return lume_native_spawn_env(p, n, v, e, env);
#else
    struct lume_spawn_request request = {pointer(p), n, pointer(v), e, pointer(env)};
    return lume_app_call(LUME_SPAWN_ENV, pointer(&request), 0, 0);
#endif
}

int app_wait(int pid, struct lume_wait_status *s)
{
    return lume_app_call(LUME_WAITPID, (uint32_t)pid, pointer(s), 0);
}

void app_error(const char *what, int e)
{
    app_puts(what);
    app_puts(": error ");
    if (e < 0)
    {
        app_puts("-");
        e = -e;
    }
    app_number((uint32_t)e);
    app_puts("\n");
}
