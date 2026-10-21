#include "recovery.h"
#include "lume/abi.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct lrec monitor;
static char response[256];
static bool server, can_boot = true;
static unsigned prepared, requests;

int lume_console_getc(void)
{
    return -LUME_EAGAIN;
}

void lume_console_write(const char *data, size_t bytes)
{
    (void)data;
    (void)bytes;
}

static void send(void *context, const char *bytes, size_t count)
{
    (void)context;
    assert(count < sizeof(response));
    memcpy(response, bytes, count);
    response[count] = 0;
    char *tail = strrchr(response, ' ');
    assert(tail && strtoul(tail + 1, NULL, 16) == lrec_crc(response, (size_t)(tail - response)));
    if (server)
    {
        fwrite(bytes, 1, count, stdout);
        fflush(stdout);
    }
}

static bool prepare(void *context, const struct lrec_image *kernel, const struct lrec_image *root)
{
    (void)context;
    assert(kernel->valid && root->valid);
    ++prepared;
    return can_boot;
}

static void frame(char *out, uint32_t seq, const char *command)
{
    int n = snprintf(out, LREC_LINE, "LREC1 %08x %s", seq, command);
    assert(n > 0 && (unsigned)n + 10 < LREC_LINE);
    snprintf(out + n, LREC_LINE - (unsigned)n, " %08x\n", lrec_crc(out, (size_t)n));
}

static void feed(const char *text)
{
    while (*text)
        lrec_feed(&monitor, (unsigned char)*text++);
}

static void request(const char *command, const char *expected)
{
    char wire[LREC_LINE];
    frame(wire, ++requests, command);
    response[0] = 0;
    feed(wire);
    if (!strstr(response, expected))
    {
        fprintf(stderr, "FAIL recovery command=%s response=%s expected=%s\n", command, response,
                expected);
        exit(1);
    }
}

static void kernel(unsigned char bytes[72])
{
    memset(bytes, 0, 72);
    bytes[0] = 0x10;
    bytes[3] = 0x14;
    bytes[17] = 0x10;
    bytes[24] = 8;
    memcpy(bytes + 56, "ARMd", 4);
}

static void block(char *out, uint32_t offset, const void *bytes, size_t count)
{
    const unsigned char *data = bytes;
    int n = snprintf(out, LREC_LINE, "D %08x ", offset);
    for (size_t i = 0; i < count; ++i)
        n += snprintf(out + n, LREC_LINE - (unsigned)n, "%02x", data[i]);
}

int main(int argc, char **argv)
{
    assert(argc == 2 || (argc == 3 && strcmp(argv[1], "--server") == 0));
    server = argc == 3;
    FILE *file = fopen(argv[argc - 1], "rb");
    assert(file && !fseek(file, 0, SEEK_END));
    long size = ftell(file);
    assert(size > 0 && size <= LREC_ROOT_LIMIT && !fseek(file, 0, SEEK_SET));
    unsigned char *original = malloc((size_t)size);
    assert(original && fread(original, 1, (size_t)size, file) == (size_t)size);
    fclose(file);
    unsigned char *k[2], *r[2];
    for (unsigned i = 0; i < 2; ++i)
    {
        k[i] = calloc(1, LREC_KERNEL_LIMIT + 16);
        r[i] = calloc(1, LREC_ROOT_LIMIT + 16);
        assert(k[i] && r[i]);
        memset(k[i] + LREC_KERNEL_LIMIT, 0xa5, 16);
        memset(r[i] + LREC_ROOT_LIMIT, 0xa5, 16);
    }
    const struct lrec_io io = {NULL, send, prepare};
    lrec_init(&monitor, &io, k[0], k[1], r[0], r[1], original, (uint32_t)size);
    if (server)
    {
        int c;
        while (!monitor.run && (c = getchar()) != EOF)
            lrec_feed(&monitor, c);
        return 0;
    }
    assert(lrec_root(&monitor)->valid);
    bool has_baseline = lrec_kernel(&monitor)->valid;
    request("I", " OK INFO ");
    unsigned char image[72];
    kernel(image);
    assert(lrec_kernel_valid(image, sizeof(image)));
    for (unsigned i = 0; i < 64; ++i)
        if (i == 3 || i == 8 || i == 20 || i == 24 || i == 28 || i == 32 || i == 40 || i == 48 ||
            i == 56 || i == 60)
        {
            image[i] ^= 1;
            assert(!lrec_kernel_valid(image, sizeof(image)));
            image[i] ^= 1;
        }
    request("B K 00200001 00000000 00000000", " ERR SIZE ");
    request("B R 04000001 00000000 00000000", " ERR SIZE ");
    request("B K 00000048 00000000 00000001", " ERR SIZE ");
    request("B R 00000200 00000000 00000001", " ERR BASE ");
    request("E", " ERR STATE ");
    char command[LREC_LINE], wire[LREC_LINE];
    snprintf(command, sizeof(command), "B K %08x %08x 00000000", (unsigned)sizeof(image),
             lrec_crc(image, sizeof(image)));
    request(command, " OK DONE ");
    request(command, " ERR BUSY ");
    request("G", " ERR NOT_READY ");
    request("D ffffffff 00", " ERR RANGE ");
    request("D 00000000 0", " ERR SIZE ");
    request("D 00000000 zz", " ERR HEX ");
    request("E", " ERR IMAGE_CRC ");
    block(command, 0, image, sizeof(image));
    frame(wire, ++requests, command);
    wire[35] ^= 1;
    feed(wire);
    assert(strstr(response, " ERR FRAME_CRC "));
    request("E", " ERR IMAGE_CRC ");
    request(command, " OK DONE ");
    request("E", " OK DONE ");
    const struct lrec_image *selected = lrec_kernel(&monitor);
    assert(selected->valid && selected->bytes == sizeof(image) &&
           !memcmp(selected->data, image, sizeof(image)));
    /* A lost acknowledgement can be retried without repeating the commit. */
    frame(wire, requests, "E");
    feed(wire);
    assert(strstr(response, " OK DONE ") && selected == lrec_kernel(&monitor));
    frame(wire, requests, "I");
    feed(wire);
    assert(strstr(response, " ERR SEQUENCE "));
    request("B R 00000200 b2aa7578 00000000", " OK DONE ");
    request("E", " ERR IMAGE ");
    request("A", " OK DONE ");
    for (unsigned i = 0; i < LREC_LINE + 20; ++i)
        lrec_feed(&monitor, 'x');
    lrec_feed(&monitor, '\n');
    lrec_feed(&monitor, -1);
    lrec_feed(&monitor, '\n');
    request("I", " OK INFO ");
    can_boot = false;
    request("G", " ERR HANDOFF ");
    assert(!monitor.run && prepared == 1);
    request("S", has_baseline ? " OK DONE " : " ERR BASELINE ");
    can_boot = true;
    request("G", " OK GO ");
    assert(monitor.run && prepared == 2);
    for (unsigned slot = 0; slot < 2; ++slot)
        for (unsigned i = 0; i < 16; ++i)
            assert(k[slot][LREC_KERNEL_LIMIT + i] == 0xa5 && r[slot][LREC_ROOT_LIMIT + i] == 0xa5);
    for (unsigned i = 0; i < 2; ++i)
    {
        free(k[i]);
        free(r[i]);
    }
    free(original);
    printf("PASS_RECOVERY_PROTOCOL requests=%u baseline=%u\n", requests, (unsigned)has_baseline);
    return 0;
}
