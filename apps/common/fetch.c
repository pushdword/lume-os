#include "lib.h"
#include "lume/io.h"
#include "lume/process.h"
#include "sha256.h"
#include <stdbool.h>
static struct lume_stream_request stream;
static struct lume_sha256 hash;
static unsigned char block[4096];
static unsigned char output[65536];
static unsigned output_used;

static int net(unsigned op)
{
    return (int)call(LUME_SYS_STREAM, op, (uintptr_t)&stream, sizeof(stream));
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

static bool flush_output(int fd)
{
    unsigned written = 0;
    while (written < output_used)
    {
        int64_t result = write(fd, output + written, output_used - written);
        if (result <= 0 || (uint64_t)result > output_used - written)
            return false;
        written += (unsigned)result;
    }
    output_used = 0;
    return true;
}

static bool write_block(int fd, unsigned bytes)
{
    lume_sha256_update(&hash, block, bytes);
    for (unsigned i = 0; i < bytes; ++i)
        output[output_used++] = block[i];
    return output_used < sizeof(output) || flush_output(fd);
}

int main(int argc, char **argv)
{
    unsigned char expected[32], digest[32];
    char temporary[LUME_PATH_MAX];
    if (argc != 4 || len(argv[3]) != 64 || len(argv[2]) + 6 >= sizeof(temporary))
    {
        puts("usage: fetch IPv4:port destination SHA256\n");
        return 1;
    }
    for (unsigned i = 0; i < 32; ++i)
    {
        int a = hex(argv[3][i * 2]), b = hex(argv[3][i * 2 + 1]);
        if (a < 0 || b < 0)
            return 1;
        expected[i] = (unsigned char)(a * 16 + b);
    }
    unsigned length = (unsigned)len(argv[2]);
    for (unsigned i = 0; i < length; ++i)
        temporary[i] = argv[2][i];
    const char suffix[] = ".part";
    for (unsigned i = 0; i < sizeof(suffix); ++i)
        temporary[length + i] = suffix[i];
    stream.version = LUME_IO_VERSION;
    if (!address(argv[1]))
        return 1;
    int fd = (int)open(temporary, LUME_O_WRONLY | LUME_O_CREAT | LUME_O_EXCL);
    if (fd < 0)
    {
        puts("fetch: cannot create temporary output\n");
        return 1;
    }
    int result = 1;
    bool connected = false;
    if (net(LUME_STREAM_CONNECT) < 0)
        goto done;
    connected = true;
    static const char request[] = "LUP1 GET\n";
    for (unsigned i = 0; i < sizeof(request) - 1; ++i)
        stream.data[i] = (unsigned char)request[i];
    stream.count = sizeof(request) - 1;
    if (net(LUME_STREAM_SEND) != (int)stream.count)
        goto done;
    unsigned char header[12];
    if (!receive(header, sizeof(header)) || header[0] != 'L' || header[1] != 'U' ||
        header[2] != 'P' || header[3] != '1' || word(header + 8) != sizeof(block))
        goto done;
    uint32_t total = word(header + 4), offset = 0;
    if (total > 128u * 1024u * 1024u)
        goto done;
    lume_sha256_init(&hash);
    for (;;)
    {
        unsigned char record[8];
        if (!receive(record, sizeof(record)))
            goto done;
        uint32_t position = word(record), bytes = word(record + 4);
        if (position < offset || position > total ||
            (bytes && ((position & 4095) || position == total ||
                       bytes != (total - position > 4096 ? 4096 : total - position))) ||
            (!bytes && position != total))
            goto done;
        for (unsigned i = 0; i < sizeof(block); ++i)
            block[i] = 0;
        while (offset < position)
        {
            unsigned zeros = position - offset > 4096 ? 4096 : position - offset;
            if (!write_block(fd, zeros))
                goto done;
            offset += zeros;
        }
        if (!bytes)
            break;
        if (!receive(block, bytes) || !write_block(fd, bytes))
            goto done;
        offset += bytes;
    }
    lume_sha256_final(&hash, digest);
    for (unsigned i = 0; i < 32; ++i)
        if (digest[i] != expected[i])
            goto done;
    if (!flush_output(fd) || call(LUME_SYS_FS, LUME_FS_SYNC, (uint64_t)fd, 0) < 0 || close(fd) < 0)
        goto done;
    fd = (int)open(temporary, LUME_O_RDONLY);
    if (fd < 0)
        goto done;
    lume_sha256_init(&hash);
    for (uint32_t offset = 0; offset < total;)
    {
        unsigned bytes = total - offset > sizeof(output) ? sizeof(output) : total - offset;
        int64_t n = read(fd, output, bytes);
        if (n <= 0 || (uint64_t)n > bytes)
            goto done;
        lume_sha256_update(&hash, output, (size_t)n);
        offset += (uint32_t)n;
    }
    lume_sha256_final(&hash, digest);
    for (unsigned i = 0; i < 32; ++i)
        if (digest[i] != expected[i])
            goto done;
    if (close(fd) < 0)
        goto done;
    fd = -1;
    if (call(LUME_SYS_FS, LUME_FS_RENAME, (uintptr_t)temporary, (uintptr_t)argv[2]) < 0)
        goto done;
    puts("fetch: SHA256 and SD readback verified\n");
    result = 0;
done:
    if (connected)
        (void)net(LUME_STREAM_CLOSE);
    if (fd >= 0)
        (void)close(fd);
    if (result)
    {
        (void)call(LUME_SYS_UNLINK, (uintptr_t)temporary, 0, 0);
        puts("fetch: transfer failed\n");
    }
    return result;
}
