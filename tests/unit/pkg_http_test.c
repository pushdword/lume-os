#define LUME_USER_HOST_TEST 1
#include "../../apps/pkg/platform.c"
#include <assert.h>

static unsigned char response[17000], output[17000], expected[32];
static unsigned response_bytes, response_offset, output_bytes, sends, receives, sleeps;
static unsigned pause_reads, pause_writes, close_calls, unlinks;
static uint64_t clock_ms;
static bool stalled, disk_error, sync_error, open_error;

int64_t call(uint64_t operation, uint64_t a, uint64_t b, uint64_t c)
{
    if (operation == LUME_SYS_CLOCK)
        return (int64_t)clock_ms;
    if (operation == LUME_SYS_SLEEP)
    {
        clock_ms += a;
        ++sleeps;
        return 0;
    }
    if (operation == LUME_SYS_STREAM)
    {
        struct lume_stream_request *request = (void *)(uintptr_t)b;
        assert(c == sizeof(*request));
        if (a == LUME_STREAM_CONNECT)
        {
            request->handle = 1;
            return 0;
        }
        assert(request->handle == 1);
        if (a == LUME_STREAM_CLOSE)
        {
            ++close_calls;
            return 0;
        }
        if (a == LUME_STREAM_SEND)
        {
            ++sends;
            assert(request->count);
            if (pause_writes)
            {
                --pause_writes;
                request->count = 0;
                clock_ms += 5000;
                return -LUME_EAGAIN;
            }
            return request->count > 11 ? 11 : (int)request->count;
        }
        assert(a == LUME_STREAM_RECEIVE && request->count == sizeof(network_buffer));
        ++receives;
        if (stalled || (pause_reads && response_offset >= 7000))
        {
            if (pause_reads)
                --pause_reads;
            request->count = 0;
            clock_ms += 5000;
            return -LUME_EAGAIN;
        }
        unsigned bytes = response_bytes - response_offset;
        if (bytes > 997)
            bytes = 997;
        memcpy(request->data, response + response_offset, bytes);
        response_offset += bytes;
        request->count = bytes;
        return (int)bytes;
    }
    if (operation == LUME_SYS_OPEN)
        return open_error ? -LUME_EEXIST : 3;
    if (operation == LUME_SYS_WRITE)
    {
        assert(a == 3);
        if (disk_error && output_bytes >= 7000)
            return -LUME_EIO;
        unsigned bytes = c > 713 ? 713 : (unsigned)c;
        assert(bytes <= sizeof(output) - output_bytes);
        memcpy(output + output_bytes, (void *)(uintptr_t)b, bytes);
        output_bytes += bytes;
        return (int)bytes;
    }
    if (operation == LUME_SYS_FS)
    {
        assert(a == LUME_FS_SYNC && b == 3);
        return sync_error ? -LUME_EIO : 0;
    }
    if (operation == LUME_SYS_CLOSE)
    {
        assert(a == 3);
        return 0;
    }
    assert(operation == LUME_SYS_UNLINK);
    ++unlinks;
    return 0;
}

static void reset(void)
{
    static const char header[] = "HTTP/1.1 200 OK\r\nContent-Length: 16385\r\n\r\n";
    memcpy(response, header, sizeof(header) - 1);
    for (unsigned i = 0; i < 16385; ++i)
        response[sizeof(header) - 1 + i] = (unsigned char)(i * 37u);
    response_bytes = sizeof(header) - 1 + 16385;
    struct lume_sha256 hash;
    lume_sha256_init(&hash);
    lume_sha256_update(&hash, response + sizeof(header) - 1, 16385);
    lume_sha256_final(&hash, expected);
    response_offset = output_bytes = sends = receives = sleeps = 0;
    pause_reads = pause_writes = close_calls = unlinks = 0;
    clock_ms = 0;
    stalled = disk_error = sync_error = open_error = false;
}

static int download(void)
{
    return pkg_download("http://172.30.1.140:8080/repo/payload.lpk", "/data/cache/payload", 16385,
                        expected);
}

int main(void)
{
    reset();
    pause_reads = pause_writes = 2;
    assert(!download() && output_bytes == 16385 && sleeps == 4 && close_calls == 1 && !unlinks);
    assert(!memcmp(output, response + response_bytes - 16385, 16385));
    reset();
    stalled = true;
    assert(download() && receives == 6 && clock_ms >= 30000 && clock_ms < 31000);
    assert(close_calls == 1 && !unlinks && !output_bytes);
    reset();
    response_bytes -= 7;
    assert(download() && output_bytes == 16378 && !sleeps && unlinks == 1);
    reset();
    expected[0] ^= 1;
    assert(download() && output_bytes == 16385 && unlinks == 1);
    reset();
    disk_error = true;
    assert(download() && output_bytes < 16385 && unlinks == 1);
    reset();
    sync_error = true;
    assert(download() && output_bytes == 16385 && unlinks == 1);
    reset();
    open_error = true;
    assert(download() && !output_bytes && !unlinks);
    reset();
    assert(pkg_download("https://172.30.1.140/payload", "/data/cache/payload", 16385, expected));
    assert(!sends && !receives && !unlinks);
    return 0;
}
