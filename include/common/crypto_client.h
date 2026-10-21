#ifndef LUME_CRYPTO_CLIENT_H
#define LUME_CRYPTO_CLIENT_H
#include "api.h"
#include "crypto.h"

static inline int app_crypto(unsigned op, void *buffer, unsigned n)
{
    struct lume_crypto_request r = {LUME_CRYPTO_VERSION, sizeof r, 0, (uint32_t)(uintptr_t)buffer,
                                    n};
    return lume_syscall(LUME_CRYPTO, op, (uint32_t)(uintptr_t)&r, sizeof r);
}
#endif
