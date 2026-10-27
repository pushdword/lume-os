#include "../net/network_tool.h"
#include "lume/process.h"
#include "lume/file_mode.h"
#include "pkg_format.h"
#include "sha256.h"
#include <string.h>

static struct lume_stream_request stream;
static unsigned char network_buffer[4096];
static unsigned received, consumed;

int pkg_payload_mode(int fd, unsigned mode)
{
    struct lume_file_mode_request request = {
        .version = LUME_FILE_MODE_VERSION, .bytes = sizeof(request), .fd = fd, .mode = mode};
    int64_t result =
        call(LUME_SYS_FILE_MODE, LUME_FILE_MODE_FD, (uintptr_t)&request, sizeof(request));
    return result == -LUME_ENOSYS ? 0 : result < 0 ? -1 : 0;
}

unsigned pkg_current_pid(void)
{
    int64_t result = call(LUME_SYS_GETPID, 0, 0, 0);
    return result > 0 ? (unsigned)result : 0;
}

unsigned pkg_current_abi(void)
{
    return (unsigned)call(LUME_SYS_ABI, 0, 0, 0);
}

int pkg_owner_alive(unsigned pid)
{
    if (!pid || pid == pkg_current_pid())
        return 0;
    struct lume_task_info info;
    int64_t result = call(LUME_SYS_PROCESS, pid - 1, (uintptr_t)&info, sizeof(info));
    return result < 0 ? -1 : result != 0;
}

static int network(unsigned operation)
{
    return (int)call(LUME_SYS_STREAM, operation, (uintptr_t)&stream, sizeof(stream));
}

static int network_wait(unsigned operation)
{
    uint64_t start = tool_now();
    unsigned count = stream.count;
    for (;;)
    {
        stream.count = count;
        int result = network(operation);
        if (result != -LUME_EAGAIN || tool_now() - start >= 30000)
            return result;
        tool_sleep(10);
    }
}

static int fill_buffer(void)
{
    if (consumed == received)
    {
        stream.count = sizeof(network_buffer);
        int count = network_wait(LUME_STREAM_RECEIVE);
        if (count <= 0 || (unsigned)count > stream.count ||
            (unsigned)count > sizeof(network_buffer))
            return -1;
        memcpy(network_buffer, stream.data, (unsigned)count);
        received = (unsigned)count;
        consumed = 0;
    }
    return 0;
}

static int next_byte(void)
{
    return fill_buffer() ? -1 : network_buffer[consumed++];
}

static int line(char *out, unsigned capacity, unsigned *header_bytes)
{
    unsigned used = 0;
    for (;;)
    {
        int c = next_byte();
        if (c < 0 || ++*header_bytes > 8192 || used + 1 >= capacity)
            return -1;
        if (c == '\r')
        {
            if (next_byte() != '\n')
                return -1;
            ++*header_bytes;
            out[used] = 0;
            return 0;
        }
        if (c < 32 || c > 126)
            return -1;
        out[used++] = (char)c;
    }
}

static bool prefix(const char *value, const char *expected)
{
    while (*expected)
    {
        unsigned c = (unsigned char)*value++;
        if (c >= 'A' && c <= 'Z')
            c += 'a' - 'A';
        if (c != (unsigned char)*expected++)
            return false;
    }
    return true;
}

static int url(const char *source, char host[128], unsigned *port, const char **path)
{
    if (strncmp(source, "http://", 7))
        return -1;
    source += 7;
    unsigned n = 0;
    while (*source && *source != '/' && *source != ':')
    {
        unsigned c = (unsigned char)*source++;
        if (n + 1 >= 128 || !((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                              (c >= '0' && c <= '9') || c == '.' || c == '-'))
            return -1;
        host[n++] = (char)c;
    }
    host[n] = 0;
    *port = 80;
    if (*source == ':')
    {
        ++source;
        unsigned value = 0, digits = 0;
        while (*source >= '0' && *source <= '9')
        {
            value = value * 10 + (unsigned)(*source++ - '0');
            if (++digits > 5 || value > 65535)
                return -1;
        }
        if (!digits || !value)
            return -1;
        *port = value;
    }
    if (!n || *source != '/')
        return -1;
    *path = source;
    for (; *source; ++source)
    {
        unsigned c = (unsigned char)*source;
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
              c == '/' || c == '.' || c == '-' || c == '_'))
            return -1;
    }
    return 0;
}

int pkg_download(const char *source, const char *target, uint32_t expected_bytes,
                 const unsigned char expected[32])
{
    char host[128];
    const char *path;
    unsigned port;
    if (url(source, host, &port, &path))
        return -1;
    memset(&stream, 0, sizeof(stream));
    stream.version = LUME_IO_VERSION;
    stream.port = port;
    if (!tool_resolve(host, stream.address, false) || network(LUME_STREAM_CONNECT) < 0)
        return -1;
    char request[512];
    unsigned used = 0;
    const char *fragments[] = {"GET ", path, " HTTP/1.0\r\nHost: ", host,
                               "\r\nConnection: close\r\n\r\n"};
    int result = -1, fd = -1;
    for (unsigned i = 0; i < sizeof(fragments) / sizeof(fragments[0]); ++i)
    {
        size_t bytes = len(fragments[i]);
        if (bytes > sizeof(request) - used)
            goto done;
        memcpy(request + used, fragments[i], bytes);
        used += (unsigned)bytes;
    }
    for (unsigned offset = 0; offset < used;)
    {
        stream.count = used - offset;
        memcpy(stream.data, request + offset, stream.count);
        int sent = network_wait(LUME_STREAM_SEND);
        if (sent <= 0 || (unsigned)sent > stream.count)
            goto done;
        offset += (unsigned)sent;
    }
    received = consumed = 0;
    char header[512];
    unsigned header_bytes = 0;
    if (line(header, sizeof(header), &header_bytes) ||
        (strncmp(header, "HTTP/1.0 200 ", 13) && strncmp(header, "HTTP/1.1 200 ", 13)))
        goto done;
    bool have_length = false;
    unsigned bytes = 0;
    for (;;)
    {
        if (line(header, sizeof(header), &header_bytes))
            goto done;
        if (!*header)
            break;
        if (prefix(header, "transfer-encoding:") || prefix(header, "content-encoding:"))
            goto done;
        if (prefix(header, "content-length:"))
        {
            const char *value = header + 15;
            while (*value == ' ')
                ++value;
            if (have_length || !tool_unsigned(value, PKG_BYTES_MAX, &bytes))
                goto done;
            have_length = true;
        }
    }
    if (!have_length || (expected_bytes && bytes != expected_bytes) ||
        (!expected_bytes && (bytes < 32 || bytes > sizeof(struct pkg_catalog))))
        goto done;
    fd = (int)open(target, LUME_O_WRONLY | LUME_O_CREAT | LUME_O_EXCL);
    if (fd < 0)
        goto done;
    struct lume_sha256 hash;
    lume_sha256_init(&hash);
    unsigned remaining = bytes;
    while (remaining)
    {
        if (fill_buffer())
            goto done;
        unsigned count = received - consumed;
        if (count > remaining)
            count = remaining;
        lume_sha256_update(&hash, network_buffer + consumed, count);
        for (unsigned offset = 0; offset < count;)
        {
            int64_t n = write(fd, network_buffer + consumed + offset, count - offset);
            if (n <= 0 || (uint64_t)n > count - offset)
                goto done;
            offset += (unsigned)n;
        }
        consumed += count;
        remaining -= count;
    }
    unsigned char digest[32];
    lume_sha256_final(&hash, digest);
    if (memcmp(digest, expected, 32) || call(LUME_SYS_FS, LUME_FS_SYNC, (unsigned)fd, 0) < 0)
        goto done;
    result = 0;
done:
    if (fd >= 0 && close(fd) < 0)
        result = -1;
    if (network(LUME_STREAM_CLOSE) < 0)
        result = -1;
    if (result && fd >= 0)
        (void)call(LUME_SYS_UNLINK, (uintptr_t)target, 0, 0);
    return result;
}
