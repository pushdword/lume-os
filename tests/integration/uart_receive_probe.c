#include "lume/app_support.h"
#include "fd.h"
#include <stdint.h>

static unsigned char block[4096], check[4096];
static char path[128] = "/data/uart-receive-test/uartio";

static int tx_child(void)
{
    for (unsigned i = 0; i < 64; ++i)
        block[i] = i == 63 ? '\n' : '~';
    for (unsigned i = 0; i < 256; ++i)
    {
        if (write_all(1, block, 64) != 64 || call(LUME_SYS_SLEEP, 1, 0, 0))
            return 1;
    }
    puts("UART_TX_DONE\n");
    return 0;
}

static int io_child(void)
{
    for (unsigned i = 0; i < sizeof(block); ++i)
        block[i] = (unsigned char)(i * 31u);
    for (unsigned pass = 0; pass < 8; ++pass)
    {
        int fd = (int)call(LUME_SYS_OPEN, (uintptr_t)path,
                           LUME_O_WRONLY | LUME_O_CREAT | LUME_O_TRUNC, 0);
        if (fd < 0)
            return 1;
        for (unsigned i = 0; i < 32; ++i)
            if (write_all(fd, block, sizeof(block)) != sizeof(block))
                return 2;
        if (call(LUME_SYS_CLOSE, fd, 0, 0))
            return 3;
        fd = (int)call(LUME_SYS_OPEN, (uintptr_t)path, LUME_O_RDONLY, 0);
        if (fd < 0)
            return 4;
        for (unsigned i = 0; i < 32; ++i)
        {
            if (read(fd, check, sizeof(check)) != sizeof(check))
                return 5;
            for (unsigned k = 0; k < sizeof(check); ++k)
                if (check[k] != block[k])
                    return 6;
        }
        if (call(LUME_SYS_CLOSE, fd, 0, 0))
            return 7;
    }
    if (call(LUME_SYS_UNLINK, (uintptr_t)path, 0, 0))
        return 8;
    puts("UART_IO_DONE\n");
    return 0;
}

static int byte(void)
{
    unsigned char value;
    uint64_t deadline = (uint64_t)call(LUME_SYS_CLOCK, 0, 0, 0) + 10000;
    do
    {
        int64_t result = read(0, &value, 1);
        if (result == 1)
            return value;
        if (result != -LUME_EAGAIN || (uint64_t)call(LUME_SYS_CLOCK, 0, 0, 0) >= deadline)
            return -1;
        (void)call(LUME_SYS_SLEEP, 1, 0, 0);
    } while (1);
}

int main(int argc, char **argv)
{
    if (argc > 2)
    {
        size_t bytes = len(argv[2]);
        static const char suffix[] = "/uartio";
        if (argv[2][0] != '/' || bytes + sizeof(suffix) > sizeof(path))
            return 1;
        for (size_t i = 0; i < bytes; ++i)
            path[i] = argv[2][i];
        for (size_t i = 0; i < sizeof(suffix); ++i)
            path[bytes + i] = suffix[i];
    }
    int64_t child = 0;
    if (argc > 1)
    {
        child = call(LUME_SYS_FORK, 0, 0, 0);
        if (child < 0)
            return 1;
        if (!child)
            return argv[1][0] == 't' ? tx_child() : io_child();
    }
    for (unsigned i = 0; i < 16; ++i)
    {
        char arm[] = "UART_RX_ARM_A\n", ack[] = "UART_RX_ACK_A\n";
        arm[12] = ack[12] = (char)('A' + i);
        puts(arm);
        if (byte() != (int)('A' + i))
            return 2;
        puts(ack);
    }
    puts("UART_RX_BURST_READY\n");
    unsigned errors = 0;
    for (unsigned i = 0; i < 8192; ++i)
    {
        int value = byte();
        if (value < 0)
            return 3;
        errors += value != (int)(33u + i % 90u);
    }
    if (errors)
        return 4;
    if (child)
    {
        int64_t result;
        do
        {
            result = call(LUME_SYS_WAIT, (uint64_t)child, 1, 0);
            if (result == -LUME_EAGAIN)
                (void)call(LUME_SYS_SLEEP, 10, 0, 0);
        } while (result == -LUME_EAGAIN || result == -4);
        if (result < 0 || (uint32_t)result || (uint32_t)((uint64_t)result >> 32) != child)
            return 5;
    }
    puts("UART_RX_PASS bytes=8192 wakeups=16\n");
    return 0;
}
