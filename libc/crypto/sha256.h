#ifndef LUME_SHA256_H
#define LUME_SHA256_H
#include <stddef.h>
#include <stdint.h>

struct lume_sha256
{
    uint32_t h[8];
    uint64_t bytes;
    unsigned used;
    unsigned char block[64];
};

void lume_sha256_init(struct lume_sha256 *);
void lume_sha256_update(struct lume_sha256 *, const void *, size_t);
void lume_sha256_final(struct lume_sha256 *, unsigned char[32]);
#endif
