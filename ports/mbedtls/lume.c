/* SPDX-License-Identifier: BSD-3-Clause */
#include "lume_tls_config.h"
#include "lume/syscall_native.h"
#include "lume/abi.h"
#include <mbedtls/entropy.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl.h>
#include <mbedtls/platform_time.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/pk.h>
#include <mbedtls/platform_util.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int mbedtls_hardware_poll(void *context, unsigned char *output, size_t bytes, size_t *received)
{
    (void)context;
    *received = 0;
    while (*received < bytes)
    {
        size_t count = bytes - *received > 256 ? 256 : bytes - *received;
        int64_t result =
            lume_native_call(LUME_SYS_RANDOM, (uintptr_t)(output + *received), count, 0);
        if (result < 0 || (uint64_t)result != count)
            return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
        *received += count;
    }
    return 0;
}

time_t lume_tls_time(time_t *result)
{
    struct timespec now;
    time_t seconds = clock_gettime(CLOCK_REALTIME, &now) ? (time_t)-1 : now.tv_sec;
    if (result)
        *result = seconds;
    return seconds;
}

int lume_tls_parse_file(mbedtls_x509_crt *chain, const char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return MBEDTLS_ERR_PK_FILE_IO_ERROR;
    struct stat metadata;
    if (fstat(fd, &metadata) || !S_ISREG(metadata.st_mode) || metadata.st_size <= 0 ||
        metadata.st_size > 4 * 1024 * 1024)
    {
        close(fd);
        return MBEDTLS_ERR_PK_FILE_IO_ERROR;
    }
    size_t bytes = (size_t)metadata.st_size, done = 0;
    unsigned char *buffer = calloc(bytes + 1, 1);
    if (!buffer)
    {
        close(fd);
        return MBEDTLS_ERR_PK_ALLOC_FAILED;
    }
    while (done < bytes)
    {
        size_t count = bytes - done > 16384 ? 16384 : bytes - done;
        ssize_t received = read(fd, buffer + done, count);
        if (received < 0 && errno == EINTR)
            continue;
        if (received <= 0)
            break;
        done += (size_t)received;
    }
    close(fd);
    int result = MBEDTLS_ERR_PK_FILE_IO_ERROR;
    if (done == bytes)
        result = mbedtls_x509_crt_parse(chain, buffer,
                                        bytes + (strstr((char *)buffer, "-----BEGIN ") != NULL));
    mbedtls_platform_zeroize(buffer, bytes + 1);
    free(buffer);
    return result;
}

mbedtls_ms_time_t mbedtls_ms_time(void)
{
    return lume_native_call(LUME_SYS_CLOCK, 0, 0, 0);
}

int mbedtls_net_recv(void *context, unsigned char *buffer, size_t bytes)
{
    int fd = ((mbedtls_net_context *)context)->fd;
    ssize_t result = read(fd, buffer, bytes);
    if (result >= 0)
        return (int)result;
    if (errno == EAGAIN || errno == EINTR)
        return MBEDTLS_ERR_SSL_WANT_READ;
    if (errno == EPIPE || errno == ECONNRESET)
        return MBEDTLS_ERR_NET_CONN_RESET;
    return MBEDTLS_ERR_NET_RECV_FAILED;
}

int mbedtls_net_send(void *context, const unsigned char *buffer, size_t bytes)
{
    int fd = ((mbedtls_net_context *)context)->fd;
    ssize_t result = write(fd, buffer, bytes);
    if (result >= 0)
        return (int)result;
    if (errno == EAGAIN || errno == EINTR)
        return MBEDTLS_ERR_SSL_WANT_WRITE;
    if (errno == EPIPE || errno == ECONNRESET)
        return MBEDTLS_ERR_NET_CONN_RESET;
    return MBEDTLS_ERR_NET_SEND_FAILED;
}
