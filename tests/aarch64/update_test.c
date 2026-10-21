#define LUME_USER_HOST_TEST 1
#define main updater_main
#define puts user_puts
#include "../../apps/common/update.c"
#undef puts
#undef main
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned char wire[9000], image[1048576], installed[1048576];
static unsigned wire_bytes, cursor, written, activations, aborted, closed;
static bool verify_failure;

static void put(unsigned char *p, unsigned n)
{
    for (unsigned i = 0; i < 4; ++i)
        p[i] = (unsigned char)(n >> (8 * i));
}

int64_t call(uint64_t number, uint64_t op, uint64_t pointer, uint64_t bytes)
{
    if (number == LUME_SYS_WRITE)
        return (int64_t)bytes;
    if (number == LUME_SYS_STREAM)
    {
        struct lume_stream_request *r = (void *)(uintptr_t)pointer;
        assert(bytes == sizeof(*r));
        if (op == LUME_STREAM_CONNECT)
        {
            r->handle = 7;
            return 0;
        }
        if (op == LUME_STREAM_SEND)
            return r->count;
        if (op == LUME_STREAM_CLOSE)
        {
            ++closed;
            return 0;
        }
        unsigned n = wire_bytes - cursor;
        if (n > r->count)
            n = r->count;
        if (n > 113)
            n = 113;
        memcpy(r->data, wire + cursor, n);
        cursor += n;
        r->count = n;
        return n;
    }
    assert(number == LUME_SYS_BOOT);
    struct lume_boot_request *r = (void *)(uintptr_t)pointer;
    assert(bytes == sizeof(*r));
    if (op == LUME_BOOT_BEGIN)
    {
        assert(r->total == sizeof(image));
        r->offset = 0;
        return 0;
    }
    if (op == LUME_BOOT_WRITE)
    {
        assert(r->offset == written && r->count <= sizeof(image) - written);
        memcpy(installed + written, r->data, r->count);
        written += r->count;
        r->offset = written;
        return r->count;
    }
    if (op == LUME_BOOT_VERIFY)
    {
        assert(written == sizeof(image) && !memcmp(installed, image, sizeof(image)));
        return verify_failure ? -LUME_EIO : 0;
    }
    if (op == LUME_BOOT_ACTIVATE)
    {
        ++activations;
        r->active = 1;
        return 0;
    }
    if (op == LUME_BOOT_ABORT)
    {
        ++aborted;
        return 0;
    }
    assert(0);
    return -1;
}

static char expected[65];

static void reset(void)
{
    memset(&stream, 0, sizeof(stream));
    memset(&boot, 0, sizeof(boot));
    opened = staging = false;
    cursor = written = activations = aborted = closed = 0;
    verify_failure = false;
    memset(image, 0, sizeof(image));
    for (unsigned i = 0; i < 4096; ++i)
        image[i] = (unsigned char)(i * 17);
    memset(image + sizeof(image) - 4096, 0x73, 4096);
    memcpy(wire, "LUP1", 4);
    put(wire + 4, sizeof(image));
    put(wire + 8, 4096);
    put(wire + 12, 0);
    put(wire + 16, 4096);
    memcpy(wire + 20, image, 4096);
    put(wire + 4116, sizeof(image) - 4096);
    put(wire + 4120, 4096);
    memcpy(wire + 4124, image + sizeof(image) - 4096, 4096);
    put(wire + 8220, sizeof(image));
    put(wire + 8224, 0);
    wire_bytes = 8228;
    struct lume_sha256 s;
    unsigned char digest[32];
    lume_sha256_init(&s);
    lume_sha256_update(&s, image, sizeof(image));
    lume_sha256_final(&s, digest);
    for (unsigned i = 0; i < 32; ++i)
        sprintf(expected + 2 * i, "%02x", digest[i]);
}

int main(void)
{
    char *args[] = {"update", "172.30.1.163:8095", expected};
    reset();
    assert(updater_main(3, args) == 0 && activations == 1 && !aborted && closed == 1);
    reset();
    expected[0] = expected[0] == '0' ? '1' : '0';
    assert(updater_main(3, args) == 1 && !activations && aborted == 1 && closed == 1);
    reset();
    put(wire + 4116, 0);
    assert(updater_main(3, args) == 1 && !activations && aborted == 1);
    reset();
    wire_bytes = 500;
    assert(updater_main(3, args) == 1 && !activations && aborted == 1 && closed == 1);
    reset();
    wire[20] ^= 1;
    assert(updater_main(3, args) == 1 && !activations && aborted == 1);
    reset();
    verify_failure = true;
    assert(updater_main(3, args) == 1 && !activations && aborted == 1);
    reset();
    wire[0] = 'X';
    assert(updater_main(3, args) == 1 && !activations && !aborted && closed == 1);
    puts(
        "PASS updater fragmented TCP, sparse gaps, SHA rejection, malformed/truncated stream and readback failure");
}
