#include "recovery.h"
#include "lume/fat.h"
#include "lume/vfs.h"

static void copy(void *to, const void *from, size_t count)
{
    unsigned char *d = to;
    const unsigned char *s = from;
    while (count--)
        *d++ = *s++;
}

static bool same(const char *a, const char *b)
{
    while (*a && *a == *b)
    {
        ++a;
        ++b;
    }
    return *a == *b;
}

uint32_t lrec_crc(const void *data, size_t count)
{
    const unsigned char *p = data;
    uint32_t crc = UINT32_MAX;
    while (count--)
    {
        crc ^= *p++;
        for (unsigned i = 0; i < 8; ++i)
            crc = (crc >> 1) ^ ((0u - (crc & 1)) & UINT32_C(0xedb88320));
    }
    return ~crc;
}

static uint32_t word(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

bool lrec_kernel_valid(const void *data, size_t bytes)
{
    const unsigned char *p = data;
    if (bytes < 68 || bytes > LREC_KERNEL_LIMIT || word(p + 56) != UINT32_C(0x644d5241) ||
        (word(p) & UINT32_C(0xfc000000)) != UINT32_C(0x14000000))
        return false;
    uint32_t target = (word(p) & UINT32_C(0x03ffffff)) * 4;
    if (target < 64 || target >= bytes || word(p + 16) < bytes ||
        word(p + 16) > LREC_KERNEL_LIMIT || word(p + 24) != 8)
        return false;
    const unsigned fields[] = {8, 12, 20, 28, 32, 36, 40, 44, 48, 52, 60};
    for (unsigned i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i)
        if (word(p + fields[i]))
            return false;
    return true;
}

bool lrec_root_valid(const void *data, size_t bytes)
{
    if (!bytes || bytes > LREC_ROOT_LIMIT)
        return false;
    /* Use the kernel's actual import rules, including its embedded shell and
     * node limits, rather than accepting only a syntactically valid FAT. */
    lume_vfs_init(NULL, 0);
    return lume_vfs_mount_fat(data, bytes);
}

const struct lrec_image *lrec_kernel(const struct lrec *s)
{
    return &s->kernel[s->selected_kernel];
}

const struct lrec_image *lrec_root(const struct lrec *s)
{
    return &s->root[s->selected_root];
}

bool lrec_start(struct lrec *s)
{
    if (s->run || s->pending || !lrec_kernel(s)->valid || !lrec_root(s)->valid ||
        !s->io.prepare(s->io.context, lrec_kernel(s), lrec_root(s)))
        return false;
    s->run = true;
    return true;
}

struct baseline
{
    struct lume_fat_file file;
    bool found;
};

static bool find_kernel(const char *path, struct lume_fat_file file, void *context)
{
    struct baseline *b = context;
    if (same(path, "boot/kernel.img") && !file.directory)
    {
        b->file = file;
        b->found = true;
    }
    return true;
}

static bool baseline(struct lrec *s)
{
    const struct lrec_image *root = &s->root[2];
    struct lume_fat fat;
    struct baseline b = {0};
    if (!root->valid || !lume_fat_mount(&fat, root->data, root->bytes) ||
        !lume_fat_walk(&fat, find_kernel, &b) || !b.found || b.file.size > LREC_KERNEL_LIMIT)
        return false;
    unsigned slot = 1 - s->selected_kernel;
    struct lrec_image *kernel = &s->kernel[slot];
    if (lume_fat_read(&fat, b.file, 0, kernel->data, b.file.size) ||
        !lrec_kernel_valid(kernel->data, b.file.size))
        return false;
    kernel->bytes = b.file.size;
    kernel->crc = lrec_crc(kernel->data, kernel->bytes);
    kernel->valid = true;
    s->selected_kernel = slot;
    s->selected_root = 2;
    s->pending = false;
    return true;
}

void lrec_init(struct lrec *s, const struct lrec_io *io, unsigned char *kernel0,
               unsigned char *kernel1, unsigned char *root0, unsigned char *root1,
               unsigned char *initial_root, uint32_t root_bytes)
{
    *s = (struct lrec){0};
    s->io = *io;
    s->kernel[0].data = kernel0;
    s->kernel[1].data = kernel1;
    s->root[0].data = root0;
    s->root[1].data = root1;
    s->root[2] = (struct lrec_image){initial_root, root_bytes, 0, false};
    s->selected_root = 2;
    if (root_bytes && root_bytes <= LREC_ROOT_LIMIT)
    {
        s->root[2].crc = lrec_crc(initial_root, root_bytes);
        s->root[2].valid = lrec_root_valid(initial_root, root_bytes);
        (void)baseline(s);
    }
}

static size_t text(char *buffer, size_t at, const char *string)
{
    while (*string)
        buffer[at++] = *string++;
    return at;
}

static size_t hexadecimal(char *buffer, size_t at, uint32_t value)
{
    const char *digits = "0123456789abcdef";
    for (unsigned i = 0; i < 8; ++i)
        buffer[at++] = digits[(value >> (28 - 4 * i)) & 15];
    return at;
}

static int digit(unsigned char c)
{
    return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
}

static bool number(const char *p, uint32_t *value)
{
    uint32_t n = 0;
    for (unsigned i = 0; i < 8; ++i)
    {
        int d = digit((unsigned char)p[i]);
        if (d < 0)
            return false;
        n = (n << 4) | (unsigned)d;
    }
    if (p[8])
        return false;
    *value = n;
    return true;
}

static void reply(struct lrec *s, uint32_t seq, uint32_t crc, bool ok, const char *message,
                  bool cache)
{
    char buffer[192];
    size_t n = text(buffer, 0, "LREC1 ");
    n = hexadecimal(buffer, n, seq);
    n = text(buffer, n, ok ? " OK " : " ERR ");
    n = text(buffer, n, message);
    uint32_t checksum = lrec_crc(buffer, n);
    buffer[n++] = ' ';
    n = hexadecimal(buffer, n, checksum);
    buffer[n++] = '\n';
    if (cache)
    {
        copy(s->reply, buffer, n);
        s->reply_bytes = n;
        s->last_seq = seq;
        s->last_crc = crc;
        s->cached = true;
    }
    s->io.send(s->io.context, buffer, n);
}

static const char *begin(struct lrec *s, char **args, unsigned count)
{
    uint32_t size, crc, base;
    if (count != 7 || (!same(args[3], "K") && !same(args[3], "R")) || !number(args[4], &size) ||
        !number(args[5], &crc) || !number(args[6], &base))
        return "ARG";
    if (s->pending)
        return "BUSY";
    unsigned kind = same(args[3], "R");
    if (!size || size > (kind ? LREC_ROOT_LIMIT : LREC_KERNEL_LIMIT) || (!kind && base))
        return "SIZE";
    const struct lrec_image *current = lrec_root(s);
    if (base && (!kind || current->bytes != size || current->crc != base))
        return "BASE";
    unsigned slot = kind ? (s->selected_root == 0 ? 1 : 0) : 1 - s->selected_kernel;
    struct lrec_image *image = kind ? &s->root[slot] : &s->kernel[slot];
    image->valid = false;
    if (base)
        copy(image->data, current->data, size);
    else
        for (uint32_t i = 0; i < size; ++i)
            image->data[i] = 0;
    s->pending = true;
    s->pending_kind = kind;
    s->pending_slot = slot;
    s->expected_bytes = size;
    s->expected_crc = crc;
    return NULL;
}

static const char *data(struct lrec *s, char **args, unsigned count)
{
    uint32_t offset;
    if (count != 5 || !number(args[3], &offset))
        return "ARG";
    if (!s->pending)
        return "STATE";
    unsigned char decoded[LREC_BLOCK];
    unsigned size = 0;
    const char *hex = args[4];
    while (*hex)
    {
        if (size == LREC_BLOCK || !hex[1])
            return "SIZE";
        int a = digit((unsigned char)*hex++), b = digit((unsigned char)*hex++);
        if (a < 0 || b < 0)
            return "HEX";
        decoded[size++] = (unsigned char)((a << 4) | b);
    }
    if (!size || offset > s->expected_bytes || size > s->expected_bytes - offset)
        return "RANGE";
    struct lrec_image *image =
        s->pending_kind ? &s->root[s->pending_slot] : &s->kernel[s->pending_slot];
    copy(image->data + offset, decoded, size);
    return NULL;
}

static const char *end(struct lrec *s)
{
    if (!s->pending)
        return "STATE";
    struct lrec_image *image =
        s->pending_kind ? &s->root[s->pending_slot] : &s->kernel[s->pending_slot];
    if (lrec_crc(image->data, s->expected_bytes) != s->expected_crc)
        return "IMAGE_CRC";
    bool valid = s->pending_kind ? lrec_root_valid(image->data, s->expected_bytes)
                                 : lrec_kernel_valid(image->data, s->expected_bytes);
    if (!valid)
        return "IMAGE";
    image->valid = true;
    image->bytes = s->expected_bytes;
    image->crc = s->expected_crc;
    if (s->pending_kind)
        s->selected_root = s->pending_slot;
    else
        s->selected_kernel = s->pending_slot;
    s->pending = false;
    return NULL;
}

static void process(struct lrec *s)
{
    size_t n = s->used;
    if (n && s->line[n - 1] == '\r')
        --n;
    if (n < 17 || n >= sizeof(s->line))
        return;
    s->line[n] = 0;
    size_t tail = n - 9;
    uint32_t expected;
    if (s->line[tail] != ' ' || !number(s->line + tail + 1, &expected))
        return;
    uint32_t actual = lrec_crc(s->line, tail);
    s->line[tail] = 0;
    char *args[8];
    unsigned count = 0;
    char *p = s->line;
    while (*p && count < 8)
    {
        args[count++] = p;
        while (*p && *p != ' ')
            ++p;
        if (*p)
            *p++ = 0;
    }
    uint32_t seq;
    if (count < 3 || *p || !same(args[0], "LREC1") || !number(args[1], &seq))
        return;
    if (actual != expected)
    {
        reply(s, seq, 0, false, "FRAME_CRC", false);
        return;
    }
    if (s->cached && seq == s->last_seq)
    {
        if (actual == s->last_crc)
            s->io.send(s->io.context, s->reply, s->reply_bytes);
        else
            reply(s, seq, actual, false, "SEQUENCE", false);
        return;
    }
    const char *error = "ARG", *answer = "DONE";
    char information[96];
    if (same(args[2], "I") && count == 3)
    {
        const struct lrec_image *k = lrec_kernel(s), *r = lrec_root(s);
        size_t at = text(information, 0, "INFO ");
        const uint32_t values[] = {k->bytes, k->crc, r->bytes, r->crc, k->valid, r->valid};
        for (unsigned i = 0; i < 6; ++i)
        {
            at = hexadecimal(information, at, values[i]);
            if (i != 5)
                information[at++] = ' ';
        }
        information[at] = 0;
        answer = information;
        error = NULL;
    }
    else if (same(args[2], "B"))
        error = begin(s, args, count);
    else if (same(args[2], "D"))
        error = data(s, args, count);
    else if (same(args[2], "E") && count == 3)
        error = end(s);
    else if (same(args[2], "A") && count == 3)
    {
        s->pending = false;
        error = NULL;
    }
    else if (same(args[2], "S") && count == 3)
        error = baseline(s) ? NULL : "BASELINE";
    else if (same(args[2], "G") && count == 3)
    {
        if (s->pending || !lrec_kernel(s)->valid || !lrec_root(s)->valid)
            error = "NOT_READY";
        else if (!lrec_start(s))
            error = "HANDOFF";
        else
        {
            error = NULL;
            answer = "GO";
        }
    }
    reply(s, seq, actual, error == NULL, error ? error : answer, true);
}

void lrec_feed(struct lrec *s, int byte)
{
    if (s->run)
        return;
    if (byte < 0 || byte > 127)
    {
        s->overflow = true;
        return;
    }
    if (byte == '\n')
    {
        if (!s->overflow)
            process(s);
        s->used = 0;
        s->overflow = false;
    }
    else if (s->used + 1 < sizeof(s->line))
        s->line[s->used++] = (char)byte;
    else
        s->overflow = true;
}
