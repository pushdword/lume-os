#include "api.h"
#include "includes.h"
#include "curve25519.h"
#include <string.h>

void *m_malloc(size_t size)
{
    void *result = calloc(1, size);
    if (!result)
        app_exit(10);
    return result;
}

void dropbear_exit(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    vfprintf(stderr, format, arguments);
    va_end(arguments);
    app_exit(11);
}

int ssh_crypto_initialize(void);
int ssh_aes_packet(unsigned, const unsigned char *, unsigned, const unsigned char *,
                   const unsigned char *, const unsigned char *, unsigned char *, unsigned,
                   unsigned char *);
static const unsigned char aes16[] = {3,   136, 218, 206, 96,  182, 163, 146, 243, 40,  194,
                                      185, 113, 178, 254, 120, 181, 38,  66,  47,  161, 59,
                                      26,  147, 79,  104, 146, 137, 240, 220, 179, 61};
static const unsigned char aes32[] = {206, 167, 64,  61,  77,  96,  107, 110, 7,   78, 197,
                                      211, 186, 243, 157, 24,  191, 195, 127, 82,  9,  21,
                                      66,  212, 131, 43,  134, 157, 68,  168, 219, 114};
static const unsigned char secret[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                                       11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
                                       22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
static const unsigned char public[] = {3,  161, 7,  191, 243, 206, 16,  190, 29, 112, 221,
                                       24, 231, 75, 192, 153, 103, 228, 214, 48, 155, 165,
                                       13, 95,  29, 220, 134, 100, 18,  85,  49, 184};
static const unsigned char signature[] = {
    103, 217, 215, 252, 45,  91,  160, 228, 79,  189, 26,  236, 104, 200, 74,  241,
    200, 207, 204, 203, 172, 253, 83,  140, 43,  244, 161, 12,  184, 60,  113, 183,
    25,  33,  18,  47,  46,  21,  130, 204, 93,  174, 117, 165, 99,  246, 195, 135,
    224, 205, 104, 6,   235, 235, 126, 40,  193, 139, 155, 61,  172, 39,  47,  11};
static const unsigned char xpublic[] = {143, 64,  197, 173, 182, 143, 37, 98,  74,  229, 178,
                                        20,  234, 118, 122, 110, 201, 77, 130, 157, 61,  123,
                                        94,  26,  209, 186, 111, 62,  33, 56,  40,  95};

int app_main(void)
{
    unsigned char key[32] = {0}, nonce[12] = {0}, header[4] = {0};
    unsigned char plain[16] = {0}, output[64], tag[16], scalar[32] = {9};
    if (ssh_crypto_initialize())
        return 1;
    for (unsigned bits = 0; bits < 2; ++bits)
    {
        unsigned key_bytes = bits ? 32 : 16;
        const unsigned char *expected = bits ? aes32 : aes16;
        if (ssh_aes_packet(0, key, key_bytes, nonce, header, plain, output, 16, tag) ||
            memcmp(output, expected, 16) || memcmp(tag, expected + 16, 16))
            return 2;
        if (ssh_aes_packet(1, key, key_bytes, nonce, header, expected, output, 16, tag) ||
            memcmp(output, plain, 16))
            return 3;
        for (unsigned byte = 0; byte < 16; ++byte)
        {
            memset(output, 0xa5, 16);
            tag[byte] ^= 1;
            if (!ssh_aes_packet(1, key, key_bytes, nonce, header, expected, output, 16, tag))
                return 4;
            for (unsigned i = 0; i < 16; ++i)
                if (output[i] != 0xa5)
                    return 5;
            tag[byte] ^= 1;
        }
    }
    app_puts("AES128/256-GCM vectors and authentication rejection PASS\n");
    dropbear_curve25519_scalarmult(output, secret, scalar);
    if (memcmp(output, xpublic, 32))
        return 6;
    app_puts("X25519 vector PASS\n");
    const unsigned char message[] = "R32 crypto validation";
    unsigned long size = 64;
    dropbear_ed25519_sign(message, sizeof message - 1, output, &size, secret, public);
    if (size != 64 || memcmp(output, signature, 64))
        return 7;
    if (dropbear_ed25519_verify(message, sizeof message - 1, signature, 64, public))
        return 8;
    output[0] ^= 1;
    if (!dropbear_ed25519_verify(message, sizeof message - 1, output, 64, public))
        return 9;
    app_puts("Ed25519 sign/verify and rejection PASS\nR32_CRYPTO_VECTORS_PASS\n");
    return 0;
}
