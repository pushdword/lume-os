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
void app_puts(const char *s) { (void)lume_syscall(LUME_WRITE, (uint32_t)(uintptr_t)s, app_length(s), 0); }
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
    lume_syscall(LUME_EXIT, n, 0, 0);
    for (;;)
    {
    }
}
static uint32_t pointer(const void *p) { return (uint32_t)(uintptr_t)p; }
int app_open(const char *p, unsigned f) { return lume_syscall(LUME_OPEN, pointer(p), f, 0); }
int app_close(int f) { return lume_syscall(LUME_CLOSE, (uint32_t)f, 0, 0); }
int app_read(int f, void *p, unsigned n) { return lume_syscall(LUME_READ, (uint32_t)f, pointer(p), n); }
int app_write(int f, const void *p, unsigned n) { return lume_syscall(LUME_FDWRITE, (uint32_t)f, pointer(p), n); }
int app_seek(int f, int32_t off, unsigned whence) { return lume_syscall(LUME_SEEK, (uint32_t)f, (uint32_t)off, whence); }
int app_sync(int f) { return lume_syscall(LUME_FSYNC, (uint32_t)f, 0, 0); }
int app_stat(const char *p, struct lume_stat *s) { return lume_syscall(LUME_STAT, pointer(p), pointer(s), 0); }
int app_spawn(const char *p, unsigned n, const char *const *v) { return lume_syscall(LUME_SPAWN, pointer(p), n, pointer(v)); }
int app_spawn_env(const char *p, unsigned n, const char *const *v, unsigned e, const char *const *env)
{
    struct lume_spawn_request request = {pointer(p), n, pointer(v), e, pointer(env)};
    return lume_syscall(LUME_SPAWN_ENV, pointer(&request), 0, 0);
}
int app_wait(int pid, struct lume_wait_status *s) { return lume_syscall(LUME_WAITPID, (uint32_t)pid, pointer(s), 0); }
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
