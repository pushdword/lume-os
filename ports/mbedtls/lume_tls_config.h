/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_TLS_CONFIG_H
#define LUME_TLS_CONFIG_H
#include <time.h>
#include <mbedtls/mbedtls_config.h>
#undef MBEDTLS_TIMING_C
#undef MBEDTLS_NET_C
#undef MBEDTLS_PSA_ITS_FILE_C
#undef MBEDTLS_PSA_CRYPTO_STORAGE_C
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_ENTROPY_HARDWARE_ALT
#define MBEDTLS_PLATFORM_IS_UNIXLIKE
#define MBEDTLS_PLATFORM_MS_TIME_ALT
#define MBEDTLS_NO_UDBL_DIVISION
#define MBEDTLS_PLATFORM_TIME_MACRO lume_tls_time
#ifdef __cplusplus
extern "C"
{
#endif
    time_t lume_tls_time(time_t *result);
    struct mbedtls_x509_crt;
    int lume_tls_parse_file(struct mbedtls_x509_crt *chain, const char *path);
#ifdef __cplusplus
}
#endif
#endif
