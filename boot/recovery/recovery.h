#ifndef LUME_RECOVERY_H
#define LUME_RECOVERY_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define LREC_KERNEL_LIMIT 0x400000u
#define LREC_ROOT_LIMIT 0x4000000u
#define LREC_BLOCK 512u
#define LREC_LINE 1152u
#define LREC_LOADER_ADDRESS UINT64_C(0x600000)
#define LREC_KERNEL_ADDRESS UINT64_C(0x200000)
#define LREC_KERNEL_STAGE UINT64_C(0x1000000)
#define LREC_ROOT_STAGE UINT64_C(0xc000000)

struct lrec_image
{
    unsigned char *data;
    uint32_t bytes, crc;
    bool valid;
};

struct lrec_io
{
    void *context;
    void (*send)(void *, const char *, size_t);
    bool (*prepare)(void *, const struct lrec_image *, const struct lrec_image *);
};

struct lrec
{
    struct lrec_io io;
    struct lrec_image kernel[2], root[3];
    unsigned selected_kernel, selected_root, pending_kind, pending_slot;
    bool pending, run, overflow, cached;
    uint32_t expected_bytes, expected_crc, last_seq, last_crc;
    size_t used, reply_bytes;
    char line[LREC_LINE], reply[192];
};

uint32_t lrec_crc(const void *, size_t);
bool lrec_kernel_valid(const void *, size_t);
bool lrec_root_valid(const void *, size_t);
void lrec_init(struct lrec *, const struct lrec_io *, unsigned char *kernel0,
               unsigned char *kernel1, unsigned char *root0, unsigned char *root1,
               unsigned char *initial_root, uint32_t root_bytes);
void lrec_feed(struct lrec *, int byte);
const struct lrec_image *lrec_kernel(const struct lrec *);
const struct lrec_image *lrec_root(const struct lrec *);
bool lrec_start(struct lrec *);
#endif
