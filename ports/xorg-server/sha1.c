/* SPDX-License-Identifier: BSD-3-Clause */
#include <sys/types.h>
#include <stddef.h>
#include "../../vendor/openssh/openbsd-compat/sha1.h"
#include <stdlib.h>

void *x_sha1_init(void)
{
    SHA1_CTX *context = malloc(sizeof(*context));
    if (context)
        SHA1Init(context);
    return context;
}

int x_sha1_update(void *context, void *data, int bytes)
{
    if (bytes < 0)
        return 0;
    SHA1Update(context, data, (size_t)bytes);
    return 1;
}

int x_sha1_final(void *context, unsigned char result[20])
{
    SHA1Final(result, context);
    free(context);
    return 1;
}
