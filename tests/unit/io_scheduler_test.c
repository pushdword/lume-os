#include "lume/app_support.h"
#include "lume/process.h"
#include <stdbool.h>

volatile uint64_t io_spin_progress, io_spin_stop;

static int spin(void)
{
    /* The host sets stop through the diagnostic memory interface. */
    while (!io_spin_stop)
        ++io_spin_progress;
    puts("IO_SPIN_DONE\n");
    return io_spin_progress ? 0 : 1;
}

static int clock_task(void)
{
    while (!io_spin_stop)
    {
        if (call(LUME_SYS_CLOCK, 0, 0, 0) < 0 || call(LUME_SYS_GETPID, 0, 0, 0) <= 0 ||
            call(LUME_SYS_SLEEP, 5, 0, 0))
            return 9;
        ++io_spin_progress;
    }
    puts("IO_CLOCK_DONE\n");
    return io_spin_progress ? 0 : 10;
}

static int writer(bool failure)
{
    call(LUME_SYS_SLEEP, 500, 0, 0);
    unsigned char data[4096], output[4096];
    for (unsigned i = 0; i < sizeof(data); ++i)
        data[i] = (unsigned char)(i * 37 + 19);
    int fd = (int)open("/data/io-worker/payload", LUME_O_RDWR | LUME_O_CREAT | LUME_O_TRUNC);
    int64_t result = fd;
    if (fd >= 0)
    {
        for (unsigned i = 0; i < 16; ++i)
        {
            result = write(fd, data, sizeof(data));
            if (result != sizeof(data))
                break;
        }
    }
    if (failure)
    {
        if (fd >= 0)
            close(fd);
        if (result != -LUME_EIO)
            return 2;
        puts("IO_EXPECTED_ERROR\n");
        return 0;
    }
    if (fd < 0 || result != sizeof(data) || call(LUME_SYS_SEEK, fd, 0, 0))
        return 3;
    for (unsigned n = 0; n < 16; ++n)
    {
        if (read(fd, output, sizeof(output)) != sizeof(output))
            return 4;
        for (unsigned i = 0; i < sizeof(output); ++i)
            if (output[i] != data[i])
                return 5;
    }
    if (close(fd))
        return 6;
    puts("IO_BYTES_VERIFIED\n");
    return 0;
}

static int launch(const char *path, const char *mode)
{
    const char *args[] = {path, mode, 0};
    struct lume_run_request request = {.path = (uintptr_t)path, .argv = (uintptr_t)args, .argc = 2};
    return (int)call(LUME_SYS_RUN_ENV, (uintptr_t)&request, 0, 0);
}

static int reap(unsigned pid)
{
    for (;;)
    {
        int64_t result = call(LUME_SYS_WAIT, pid, 0, 0);
        if (result != -LUME_EAGAIN)
            return result < 0 ? 99 : (int)(uint32_t)result;
        call(LUME_SYS_SLEEP, 10, 0, 0);
    }
}

int main(int argc, char **argv)
{
    if (argc == 2 && eq(argv[1], "spin"))
        return spin();
    if (argc == 2 && eq(argv[1], "clock"))
        return clock_task();
    if (argc == 2 && (eq(argv[1], "write") || eq(argv[1], "fail")))
        return writer(eq(argv[1], "fail"));
    bool failure = argc == 2 && eq(argv[1], "timeout");
    int cpu = launch(argv[0], "spin");
    int clock = launch(argv[0], "clock");
    int io = launch(argv[0], failure ? "fail" : "write");
    if (cpu < 0 || clock < 0 || io < 0)
        return 7;
    puts("IO_CONCURRENCY_READY\n");
    int io_result = reap((unsigned)io), cpu_result = reap((unsigned)cpu),
        clock_result = reap((unsigned)clock);
    if (io_result || cpu_result || clock_result)
    {
        puts("IO_CHILD_FAILED\n");
        return 8;
    }
    puts("IO_SCHEDULER_PASS\n");
    return 0;
}
