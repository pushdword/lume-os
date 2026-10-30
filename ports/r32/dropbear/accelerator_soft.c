#include "api.h"
#include "crypto_client.h"
#include "tomcrypt.h"
#include "random.h"
#include <string.h>

static int aes_cipher = -1;
static unsigned software_packets;
static unsigned char plaintext[4096];

int ssh_crypto_initialize(void)
{
    struct lume_crypto_info info;
    int result = app_crypto(LUME_CRYPTO_INFO, &info, sizeof info);
    if (result < 0)
        return result;
    if (!(info.supported & LUME_CRYPTO_HMAC_DRBG_SHA256))
        return -95;
    aes_cipher = register_cipher(&aes_desc);
    return aes_cipher < 0 ? -5 : 0;
}

/* Decryption keeps unauthenticated plaintext in a separate buffer. */
int ssh_aes_packet(unsigned decrypt, const unsigned char *key, unsigned key_bytes,
                   const unsigned char nonce[12], const unsigned char *header,
                   const unsigned char *input, unsigned char *output, unsigned bytes,
                   unsigned char *tag)
{
    unsigned char calculated[16] = {0};
    unsigned long tag_bytes = sizeof calculated;
    if (aes_cipher < 0 || bytes > sizeof plaintext || (key_bytes != 16 && key_bytes != 32))
        return -22;
    int result = gcm_memory(aes_cipher, key, key_bytes, nonce, 12, header, 4,
                            decrypt ? plaintext : (unsigned char *)input, bytes,
                            decrypt ? (unsigned char *)input : output, calculated, &tag_bytes,
                            decrypt ? GCM_DECRYPT : GCM_ENCRYPT);
    if (result != CRYPT_OK || tag_bytes != sizeof calculated)
        result = -5;
    else if (decrypt)
    {
        unsigned difference = 0;
        for (unsigned i = 0; i < sizeof calculated; ++i)
            difference |= calculated[i] ^ tag[i];
        result = difference ? -5 : 0;
        if (!result)
            memcpy(output, plaintext, bytes);
    }
    else
    {
        memcpy(tag, calculated, sizeof calculated);
        result = 0;
    }
    lume_secret_clear(calculated, sizeof calculated);
    lume_secret_clear(plaintext, bytes);
    if (!result)
        ++software_packets;
    return result;
}

void ssh_crypto_metrics(void)
{
    app_puts("SSH_AES software=");
    app_number(software_packets);
    app_puts("\nSSH_CURVE software X25519/Ed25519\n");
}
