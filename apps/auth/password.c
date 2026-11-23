/* SPDX-License-Identifier: BSD-3-Clause */
#include "password.h"
#include "auth_files.h"
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mbedtls/constant_time.h>
#include <mbedtls/pkcs5.h>
#include <mbedtls/platform_util.h>

enum
{
    salt_bytes = 16,
    digest_bytes = 32,
    iterations = 600000,
    record_name_bytes = 2 * AUTH_NAME_MAX + 10
};

static const unsigned char record_tag[] = "LUME-PBKDF2-SHA256-1";
static const char hex[] = "0123456789abcdef";

struct password_record
{
    unsigned char tag[sizeof(record_tag)];
    unsigned char salt[salt_bytes];
    unsigned char digest[digest_bytes];
};

bool auth_name_valid(const char *name)
{
    size_t length = strlen(name);
    if (!length || length > AUTH_NAME_MAX || !strcmp(name, ".") || !strcmp(name, ".."))
        return false;
    for (size_t i = 0; i < length; ++i)
    {
        unsigned char c = (unsigned char)name[i];
        if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') && !(c >= '0' && c <= '9') &&
            c != '_' && c != '-' && c != '.')
            return false;
    }
    return true;
}

static void record_name(const char *name, uint32_t uid, char path[record_name_bytes])
{
    size_t offset = 0;
    while (*name)
    {
        unsigned char c = (unsigned char)*name++;
        path[offset++] = hex[c >> 4];
        path[offset++] = hex[c & 15];
    }
    path[offset++] = '-';
    for (unsigned i = 8; i; --i)
        path[offset++] = hex[(uid >> (4 * (i - 1))) & 15];
    path[offset] = 0;
}

static int password_directory(bool create)
{
    int data = auth_trusted_directory(AT_FDCWD, "/data", false, 0755);
    if (data < 0)
        return -1;
    int etc = auth_permissions_volume(data, create)
                  ? auth_trusted_directory(data, "etc", create, 0755)
                  : -1;
    close(data);
    if (etc < 0)
        return -1;
    int directory = auth_trusted_directory(etc, "lume-auth", create, 0700);
    close(etc);
    return directory;
}

static bool private_record(int fd, struct stat *info)
{
    if (fstat(fd, info) < 0)
        return false;
    if (S_ISREG(info->st_mode) && !info->st_uid && !(info->st_mode & 077u))
        return true;
    errno = EACCES;
    return false;
}

static bool load_record(const char *name, struct password_record *record)
{
    int directory = password_directory(false);
    if (directory < 0)
        return false;
    int fd = openat(directory, name, O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC);
    close(directory);
    if (fd < 0)
        return false;
    struct stat info;
    bool valid = private_record(fd, &info) && info.st_size == sizeof(*record);
    size_t offset = 0;
    while (valid && offset < sizeof(*record))
    {
        ssize_t got = read(fd, (unsigned char *)record + offset, sizeof(*record) - offset);
        if (got > 0)
            offset += (size_t)got;
        else if (!got || errno != EINTR)
            valid = false;
    }
    close(fd);
    return valid && !memcmp(record->tag, record_tag, sizeof(record_tag));
}

static bool derive(const char *password, size_t bytes, const unsigned char salt[salt_bytes],
                   unsigned char digest[digest_bytes])
{
    if (!mbedtls_pkcs5_pbkdf2_hmac_ext(MBEDTLS_MD_SHA256, (const unsigned char *)password, bytes,
                                       salt, salt_bytes, iterations, digest_bytes, digest))
        return true;
    errno = EIO;
    return false;
}

bool auth_password_matches(const char *name, uint32_t uid, const char *password, size_t bytes)
{
    struct password_record record = {0};
    bool valid = false;
    if (name && auth_name_valid(name))
    {
        char path[record_name_bytes];
        record_name(name, uid, path);
        valid = load_record(path, &record);
    }
    unsigned char digest[digest_bytes] = {0};
    bool derived = derive(password, bytes, record.salt, digest);
    bool equal = !mbedtls_ct_memcmp(digest, record.digest, sizeof(digest));
    mbedtls_platform_zeroize(digest, sizeof(digest));
    mbedtls_platform_zeroize(&record, sizeof(record));
    return valid && derived && equal;
}

static int write_record(int directory, const char *name, const struct password_record *record)
{
    unsigned char suffix[8];
    if (!auth_random(suffix, sizeof(suffix)))
        return -1;
    char temporary[] = ".new-0000000000000000";
    for (size_t i = 0; i < sizeof(suffix); ++i)
    {
        temporary[5 + 2 * i] = hex[suffix[i] >> 4];
        temporary[6 + 2 * i] = hex[suffix[i] & 15];
    }
    int fd =
        openat(directory, temporary, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600u);
    if (fd < 0)
        return -1;
    struct stat info;
    bool written = private_record(fd, &info);
    size_t offset = 0;
    while (written && offset < sizeof(*record))
    {
        ssize_t sent = write(fd, (const unsigned char *)record + offset, sizeof(*record) - offset);
        if (sent > 0)
            offset += (size_t)sent;
        else if (!sent || errno != EINTR)
        {
            if (!sent)
                errno = EIO;
            written = false;
        }
    }
    if (written)
        written = !fsync(fd);
    int saved = written ? 0 : errno;
    if (close(fd) && !saved)
        saved = errno;
    if (!saved && renameat(directory, temporary, directory, name))
        saved = errno;
    if (saved)
    {
        (void)unlinkat(directory, temporary, 0);
        errno = saved;
        return -1;
    }
    return fsync(directory);
}

int auth_password_store(const char *name, uint32_t uid, const char *password, size_t bytes)
{
    if (!auth_name_valid(name) || bytes < AUTH_PASSWORD_MIN || bytes > AUTH_PASSWORD_MAX)
    {
        errno = EINVAL;
        return -1;
    }
    int directory = password_directory(true);
    if (directory < 0)
        return -1;
    struct password_record record = {0};
    memcpy(record.tag, record_tag, sizeof(record_tag));
    char path[record_name_bytes];
    record_name(name, uid, path);
    int result = -1;
    if (auth_random(record.salt, sizeof(record.salt)) &&
        derive(password, bytes, record.salt, record.digest))
        result = write_record(directory, path, &record);
    int saved = errno;
    mbedtls_platform_zeroize(&record, sizeof(record));
    close(directory);
    errno = saved;
    return result;
}
