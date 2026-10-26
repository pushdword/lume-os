#include "api.h"
#include <perf.h>
#include <stdint.h>
#include <string.h>

static int read_chunk(int fd, unsigned char *data, unsigned bytes)
{
    unsigned count = 0;
    while (count < bytes)
    {
        int result = app_read(fd, data + count, bytes - count);
        if (result < 0)
            return result;
        if (!result)
            break;
        count += (unsigned)result;
    }
    return (int)count;
}

int app_main(int argc, char **argv)
{
    if (argc != 4)
        return 2;
    const char *arguments[] = {argv[1], argv[2], argv[3]};
    uint32_t start = (uint32_t)lume_syscall(LUME_INFO, LUME_UPTIME, 0, 0);
    int child = app_spawn(argv[1], 3, arguments);
    struct lume_wait_status status;
    if (child < 1 || app_wait(child, &status) != child || status.code)
        return 1;
    uint32_t elapsed = (uint32_t)lume_syscall(LUME_INFO, LUME_UPTIME, 0, 0) - start;
    int source = app_open(argv[2], LUME_RDONLY);
    if (source < 0)
        return 1;
    int target = app_open(argv[3], LUME_RDONLY);
    if (target < 0)
    {
        app_close(source);
        return 1;
    }
    unsigned char expected[4096], actual[4096];
    uint32_t bytes = 0;
    int failed = 0;
    for (;;)
    {
        int count = read_chunk(source, expected, sizeof(expected));
        int copied = read_chunk(target, actual, sizeof(actual));
        if (count < 0 || copied != count || (count && memcmp(expected, actual, (unsigned)count)) ||
            (unsigned)count > UINT32_MAX - bytes)
        {
            failed = 1;
            break;
        }
        if (!count)
            break;
        bytes += (unsigned)count;
    }
    failed |= app_close(source) < 0;
    failed |= app_close(target) < 0;
    if (failed)
        return 1;
    app_puts("COPY_BENCH_PASS bytes=");
    app_number(bytes);
    app_puts(" elapsed_ms=");
    app_number(elapsed);
    app_puts("\n");
    return 0;
}
