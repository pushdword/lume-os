/*
 * Dropbear SSH
 * 
 * Copyright (c) 2002,2003 Matt Johnston
 * Copyright (c) 2020 by Vladislav Grishenko
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. */

#include "includes.h"
#include "algo.h"
#include "dbutil.h"
#include "gcm.h"
int ssh_aes_packet(unsigned, const unsigned char *, unsigned, const unsigned char *,
                   const unsigned char *, const unsigned char *, unsigned char *, unsigned,
                   unsigned char *);

#if DROPBEAR_ENABLE_GCM_MODE

#define GHASH_LEN 16

static const struct dropbear_hash dropbear_ghash = {NULL, 0, GHASH_LEN};

static int dropbear_gcm_start(int cipher, const unsigned char *IV, const unsigned char *key,
                              int keylen, int UNUSED(num_rounds), dropbear_gcm_state *state)
{

    (void)cipher;
    if (keylen != 16 && keylen != 32)
        return CRYPT_INVALID_KEYSIZE;
    memcpy(state->iv, IV, GCM_NONCE_LEN);
    memset(state->lume_key, 0, 32);
    memcpy(state->lume_key, key, keylen);
    state->lume_key_bytes = keylen;
    state->lume_exhausted = 0;
    return CRYPT_OK;
}

static int dropbear_gcm_crypt(unsigned int UNUSED(seq), const unsigned char *in, unsigned char *out,
                              unsigned long len, unsigned long taglen, dropbear_gcm_state *state,
                              int direction)
{

    if (len < 4 || len - 4 > 4096 || taglen != GHASH_LEN || state->lume_exhausted ||
        (direction != LTC_ENCRYPT && direction != LTC_DECRYPT))
        return CRYPT_ERROR;
    if (ssh_aes_packet(direction == LTC_DECRYPT, state->lume_key, state->lume_key_bytes, state->iv,
                       in, in + 4, out + 4, len - 4,
                       direction == LTC_ENCRYPT ? out + len : (unsigned char *)in + len) != 0)
        return CRYPT_ERROR;
    unsigned char *counter = state->iv + GCM_IVFIX_LEN;
    int i;
    for (i = GCM_IVCTR_LEN - 1; i >= 0 && ++counter[i] == 0; i--)
        ;
    if (i < 0)
        state->lume_exhausted = 1;
    return CRYPT_OK;
}

static int dropbear_gcm_getlength(unsigned int UNUSED(seq), const unsigned char *in,
                                  unsigned int *outlen, unsigned long len,
                                  dropbear_gcm_state *UNUSED(state))
{
    TRACE2(("enter dropbear_gcm_getlength"))

    if (len < 4)
    {
        return CRYPT_ERROR;
    }

    LOAD32H(*outlen, in);

    TRACE2(("leave dropbear_gcm_getlength"))
    return CRYPT_OK;
}

const struct dropbear_cipher_mode dropbear_mode_gcm = {
    (void *)dropbear_gcm_start,     NULL,           NULL, (void *)dropbear_gcm_crypt,
    (void *)dropbear_gcm_getlength, &dropbear_ghash};

#endif /* DROPBEAR_ENABLE_GCM_MODE */
