#include "lume_tls_config.h"
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/sha256.h>
#include <mbedtls/version.h>
#include <mbedtls/x509_crt.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int differs(const unsigned char *first, const unsigned char *second, size_t bytes)
{
    unsigned value = 0;
    for (size_t i = 0; i < bytes; ++i)
        value |= first[i] ^ second[i];
    return value != 0;
}

static int hardware_entropy(void)
{
    unsigned char first[48] = {0};
    unsigned char second[48] = {0};
    size_t received = 0;
    if (mbedtls_hardware_poll(NULL, first, sizeof(first), &received) ||
        received != sizeof(first))
        return -1;
    if (mbedtls_hardware_poll(NULL, second, sizeof(second), &received) ||
        received != sizeof(second))
        return -1;
    return differs(first, second, sizeof(first)) ? 0 : -1;
}

static int random_generator(void)
{
    static const unsigned char label[] = "Lume native mbedTLS probe";
    unsigned char first[64];
    unsigned char second[64];
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context random;
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&random);
    int result = mbedtls_ctr_drbg_seed(&random, mbedtls_entropy_func, &entropy,
                                       label, sizeof(label) - 1);
    if (!result)
        result = mbedtls_ctr_drbg_random(&random, first, sizeof(first));
    if (!result)
        result = mbedtls_ctr_drbg_random(&random, second, sizeof(second));
    if (!result && !differs(first, second, sizeof(first)))
        result = -1;
    mbedtls_ctr_drbg_free(&random);
    mbedtls_entropy_free(&entropy);
    return result;
}

static int sha256_vector(void)
{
    static const unsigned char expected[32] = {
        0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
        0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
        0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
        0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad,
    };
    unsigned char output[32];
    return mbedtls_sha256((const unsigned char *)"abc", 3, output, 0) ||
                   memcmp(output, expected, sizeof(output))
               ? -1
               : 0;
}

static int transport_callbacks(void)
{
    static const unsigned char message[] = "native transport";
    unsigned char received[sizeof(message)] = {0};
    int descriptors[2];
    if (pipe(descriptors))
        return -1;
    mbedtls_net_context writer = {.fd = descriptors[1]};
    mbedtls_net_context reader = {.fd = descriptors[0]};
    int sent = mbedtls_net_send(&writer, message, sizeof(message));
    int read_bytes = mbedtls_net_recv(&reader, received, sizeof(received));
    close(descriptors[0]);
    close(descriptors[1]);
    return sent == (int)sizeof(message) && read_bytes == (int)sizeof(message) &&
                   !memcmp(message, received, sizeof(message))
               ? 0
               : -1;
}

static int trust_store(const char *path)
{
    mbedtls_x509_crt chain;
    mbedtls_x509_crt_init(&chain);
    int result = lume_tls_parse_file(&chain, path);
    if (!result && !chain.raw.p)
        result = -1;
    mbedtls_x509_crt_free(&chain);
    return result;
}

int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    char version[18];
    mbedtls_version_get_string_full(version);
    if (strncmp(version, "Mbed TLS 3.6.7", sizeof(version)))
        return 3;
    if (lume_tls_time(NULL) < 1700000000)
        return 4;
    if (hardware_entropy())
        return 5;
    if (random_generator())
        return 6;
    if (sha256_vector())
        return 7;
    if (transport_callbacks())
        return 8;
    if (trust_store(argv[1]))
        return 9;
    printf("MBEDTLS_NATIVE_PASS version=%s entropy=PASS random=PASS sha256=PASS "
           "time=PASS transport=PASS x509=PASS\n",
           version);
    return 0;
}
