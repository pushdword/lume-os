#ifndef LUME_RANDOM_H
#define LUME_RANDOM_H
#include <crypto.h>
#define LUME_RANDOM_BUDGET (1024u * 1024u)

struct lume_random_state
{
    unsigned char key[32], value[32];
    uint32_t ready, remaining, reseeds;
};

void lume_secret_clear(void *, unsigned);
/* Invoke the scheduling hook with compression state on the caller's stack.
 * Kernel callers hold a generator lease and release queue_lock before the call. */
void lume_random_checkpoint(void);
void lume_sha256(const void *, unsigned, unsigned char[32]);
void lume_hmac_sha256(const unsigned char[32], const void *, unsigned, const void *, unsigned,
                      unsigned char[32]);
int lume_random_seed(struct lume_random_state *, const unsigned char *, unsigned);
int lume_random_bytes(struct lume_random_state *, void *, unsigned);
#endif
