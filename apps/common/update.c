#include "lib.h"
#include "lume/io.h"
#include "sha256.h"
#include <stdbool.h>
static struct lume_stream_request stream;
static struct lume_boot_request boot;
static struct lume_sha256 hash;
static bool opened, staging;

static int net(unsigned op)
{
    return (int)call(LUME_SYS_STREAM, op, (uintptr_t)&stream, sizeof(stream));
}

static int disk(unsigned op)
{
    return (int)call(LUME_SYS_BOOT, op, (uintptr_t)&boot, sizeof(boot));
}

static bool receive(void *out, unsigned count)
{
    unsigned char *p = out;
    unsigned retries = 0;
    while (count)
    {
        stream.count = count > LUME_IO_BYTES ? LUME_IO_BYTES : count;
        int n = net(LUME_STREAM_RECEIVE);
        if (n == -LUME_EAGAIN && retries++ < 3)
            continue;
        if (n <= 0 || (unsigned)n > stream.count)
            return false;
        retries = 0;
        for (int i = 0; i < n; ++i)
            *p++ = stream.data[i];
        count -= (unsigned)n;
    }
    return true;
}

static uint32_t word(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static int hex(char c)
{
    return c >= '0' && c <= '9'   ? c - '0'
           : c >= 'a' && c <= 'f' ? c - 'a' + 10
           : c >= 'A' && c <= 'F' ? c - 'A' + 10
                                  : -1;
}

static bool address(const char *p)
{
    for (unsigned i = 0; i < 5; ++i)
    {
        unsigned n = 0, digits = 0, limit = i == 4 ? 65535 : 255;
        while (*p >= '0' && *p <= '9')
        {
            if (n > limit / 10)
                return false;
            n = n * 10 + (unsigned)(*p++ - '0');
            if (n > limit || ++digits > 5)
                return false;
        }
        if (!digits)
            return false;
        if (i < 4)
            stream.address[i] = (unsigned char)n;
        else
            stream.port = n;
        if (i < 4 && *p++ != (i == 3 ? ':' : '.'))
            return false;
    }
    return !*p && stream.port;
}

static void number(unsigned n)
{
    char b[11];
    unsigned count = 0;
    do
    {
        b[count++] = (char)('0' + n % 10);
        n /= 10;
    } while (n);
    while (count)
        (void)write(1, &b[--count], 1);
}

static bool block(unsigned count)
{
    lume_sha256_update(&hash, boot.data, count);
    boot.count = count;
    return disk(LUME_BOOT_WRITE) == (int)count;
}

static int update(const char *host, const unsigned char expected[32])
{
    if (!address(host) || net(LUME_STREAM_CONNECT) < 0)
        return 1;
    opened = true;
    static const char request[] = "LUP1 GET\n";
    for (unsigned i = 0; i < sizeof(request) - 1; ++i)
        stream.data[i] = (unsigned char)request[i];
    stream.count = sizeof(request) - 1;
    if (net(LUME_STREAM_SEND) != (int)(sizeof(request) - 1))
        return 1;
    unsigned char header[12];
    if (!receive(header, sizeof(header)) || header[0] != 'L' || header[1] != 'U' ||
        header[2] != 'P' || header[3] != '1' || word(header + 8) != 4096)
        return 1;
    boot.total = word(header + 4);
    if (disk(LUME_BOOT_BEGIN) < 0)
        return 1;
    staging = true;
    puts("update: receiving kernel and programs\n");
    lume_sha256_init(&hash);
    unsigned next = 0, progress = 0;
    for (;;)
    {
        unsigned char record[8];
        if (!receive(record, 8))
            return 1;
        unsigned position = word(record), count = word(record + 4);
        if (position < next || position > boot.total || (count && (position & 4095)) ||
            (count && (position == boot.total ||
                       count != (boot.total - position > 4096 ? 4096 : boot.total - position))) ||
            (!count && position != boot.total))
            return 1;
        for (unsigned i = 0; i < 4096; ++i)
            boot.data[i] = 0;
        while (next < position)
        {
            unsigned n = position - next > 4096 ? 4096 : position - next;
            if (!block(n))
                return 1;
            next += n;
        }
        if (!count)
            break;
        if (!receive(boot.data, count) || !block(count))
            return 1;
        next += count;
        if (next / 1048576 != progress)
        {
            progress = next / 1048576;
            puts("update: written MiB ");
            number(progress);
            puts("\n");
        }
    }
    unsigned char digest[32];
    lume_sha256_final(&hash, digest);
    for (unsigned i = 0; i < 32; ++i)
        if (digest[i] != expected[i])
        {
            puts("update: SHA256 mismatch; boot selection preserved\n");
            return 1;
        }
    puts("update: SHA256 verified; checking SD readback\n");
    int result;
    do
    {
        result = disk(LUME_BOOT_VERIFY);
    } while (result == 1);
    if (result < 0 || disk(LUME_BOOT_ACTIVATE) < 0)
        return 1;
    staging = false;
    puts("update: SD readback verified; permanent slot ");
    number(boot.active);
    puts(" selected. Reboot to start it.\n");
    return 0;
}

int main(int argc, char **argv)
{
    stream.version = boot.version = LUME_IO_VERSION;
    if (argc == 2 && eq(argv[1], "status"))
    {
        if (disk(LUME_BOOT_INFO) < 0)
        {
            puts("update: boot volume unavailable\n");
            return 1;
        }
        puts("update: active slot ");
        number(boot.active);
        puts(boot.active ? " lumeslot.fat\n" : " lumeroot.fat\n");
        return 0;
    }
    unsigned char expected[32];
    if (argc != 3 || len(argv[2]) != 64)
    {
        puts("usage: update status | update IPv4:port SHA256\n");
        return 1;
    }
    for (unsigned i = 0; i < 32; ++i)
    {
        int a = hex(argv[2][2 * i]), b = hex(argv[2][2 * i + 1]);
        if (a < 0 || b < 0)
            return 1;
        expected[i] = (unsigned char)(a * 16 + b);
    }
    int result = update(argv[1], expected);
    if (staging)
        (void)disk(LUME_BOOT_ABORT);
    if (opened)
        (void)net(LUME_STREAM_CLOSE);
    if (result)
        puts("update: failed; check boot status before reboot\n");
    return result;
}
