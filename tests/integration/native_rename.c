/* SPDX-License-Identifier: BSD-3-Clause */
#include "api.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef LUME_TEST_GLIB
#include <glib.h>
#endif

static void create(const char *path, const char *contents, unsigned mode)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, mode);
    assert(fd >= 0);
    size_t bytes = strlen(contents);
    assert(write(fd, contents, bytes) == (ssize_t)bytes);
    assert(!fsync(fd) && !close(fd));
}

static void check(const char *path, const char *contents, unsigned mode)
{
    struct stat metadata;
    assert(!stat(path, &metadata) && (metadata.st_mode & 0777u) == mode);
    char data[128];
    int fd = open(path, O_RDONLY);
    assert(fd >= 0);
    size_t bytes = strlen(contents);
    assert(bytes < sizeof(data) && read(fd, data, sizeof(data)) == (ssize_t)bytes);
    assert(!memcmp(data, contents, bytes) && !close(fd));
}

static void busy(const char *opened, const char *source, const char *target)
{
    int fd = open(opened, O_RDONLY);
    assert(fd >= 0);
    errno = 0;
    int result = rename(source, target), error = errno;
    printf("NATIVE_RENAME_BUSY fd=%d result=%d error=%d expected=%d\n", fd, result, error, EBUSY);
    assert(!close(fd));
    assert(result == -1 && error == EBUSY);
}

#ifdef LUME_TEST_GLIB
static void save_settings(const char *path, const char *contents)
{
    GError *error = NULL;
    gboolean written = g_file_set_contents(path, contents, -1, &error);
    if (!written)
        fprintf(stderr, "NATIVE_GLIB_WRITE_ERROR code=%d message=%s\n", error ? error->code : -1,
                error ? error->message : "missing error");
    assert(written && !error);
}
#endif

int main(void)
{
    const char *source = "/data/rename-check/settings.new";
    const char *target = "/data/rename-check/settings.xml";
    assert(!mkdir("/data/rename-check", 0700));
    create(source, "ação nova\n", 0600);
    create(target, "configuração anterior\n", 0644);
    assert(lume_rename_noreplace(source, target) == -1 && errno == EEXIST);
    check(target, "configuração anterior\n", 0644);
    busy(source, source, target);
    busy(target, source, target);
    assert(!rename(source, source) && !rename(source, target));
    assert(access(source, F_OK) == -1 && errno == ENOENT);
    check(target, "ação nova\n", 0600);
    assert(!rename(target, source) && !rename(source, target));
    check(target, "ação nova\n", 0600);
    struct stat metadata;
    assert(!stat(target, &metadata));
    int fd = open(target, O_RDONLY);
    assert(fd >= 0 && !fchmod(fd, metadata.st_mode) && !close(fd));
    assert(!chmod(target, metadata.st_mode));
    assert(chmod(target, S_ISUID | 0600) == -1 && errno == EINVAL);
#ifdef LUME_TEST_GLIB
    const char *settings = "/data/rename-check/xfce.xml";
    save_settings(settings, "<value>primeiro</value>\n");
    assert(!chmod(settings, 0600));
    save_settings(settings, "<value>ação e informação</value>\n");
    check(settings, "<value>ação e informação</value>\n", 0600);
    save_settings(settings, "<value>último</value>\n");
    check(settings, "<value>último</value>\n", 0600);
    puts("NATIVE_GLIB_RENAME_PASS");
#endif
    puts("NATIVE_RENAME_PASS");
    return 0;
}
