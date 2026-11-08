#define _POSIX_C_SOURCE 200809L
#define LUME_PKG_HOST_TEST 1
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define lstat stat
#endif
#include "../../apps/pkg/pkg_format.h"

static struct pkg_manifest encoded;
static size_t position, available, read_calls, read_bytes;
static uint64_t archive_bytes;
static unsigned closes;
static bool stat_failure, close_failure;

static int bounds_open(const char *path, int flags, ...)
{
    assert(!strcmp(path, "archive") && flags == O_RDONLY);
    position = 0;
    return 91;
}

static ssize_t bounds_read(int fd, void *target, size_t bytes)
{
    assert(fd == 91 && bytes <= sizeof(encoded));
    ++read_calls;
    if (bytes > available - position)
        bytes = available - position;
    if (bytes > 37)
        bytes = 37;
    memcpy(target, (const unsigned char *)&encoded + position, bytes);
    position += bytes;
    read_bytes += bytes;
    return (ssize_t)bytes;
}

static int bounds_fstat(int fd, struct stat *value)
{
    assert(fd == 91);
    memset(value, 0, sizeof(*value));
    value->st_size = (off_t)archive_bytes;
    return stat_failure ? -1 : 0;
}

static int bounds_close(int fd)
{
    assert(fd == 91);
    ++closes;
    return close_failure ? -1 : 0;
}

#define open bounds_open
#define read bounds_read
#define fstat bounds_fstat
#define close bounds_close
#define main static package_unused_main
#include "../../apps/pkg/pkg.c"
#undef main
#undef open
#undef read
#undef fstat
#undef close

static void prepare(unsigned files)
{
    memset(&encoded, 0, sizeof(encoded));
    memcpy(encoded.magic, PKG_FILE_MAGIC, sizeof(encoded.magic));
    strcpy(encoded.identity.name, "boundary");
    strcpy(encoded.identity.version, "1");
    encoded.identity.revision = 1;
    encoded.identity.abi = PKG_ABI;
    encoded.identity.files = files;
    for (unsigned i = 0; i < files; ++i)
        snprintf(encoded.file[i].path, sizeof(encoded.file[i].path), "lib/file%u", i);
    available = manifest_bytes(&encoded);
    archive_bytes = available;
    read_calls = read_bytes = closes = 0;
    stat_failure = close_failure = false;
}

static void expect(bool accepted, bool archive)
{
    struct pkg_manifest decoded;
    assert((read_manifest("archive", &decoded, archive) == 0) == accepted);
    assert(closes == 1 && read_bytes <= sizeof(encoded));
    if (accepted)
        assert(!memcmp(&encoded, &decoded, available));
}

int main(void)
{
    for (int delta = -1; delta <= 1; ++delta)
    {
        prepare(1);
        encoded.file[0].bytes = PKG_BYTES_MAX - (unsigned)available + delta;
        archive_bytes = available + encoded.file[0].bytes;
        expect(delta <= 0, true);
        assert(read_bytes == available && read_calls > 2);
    }
    prepare(1);
    encoded.file[0].bytes = PKG_BYTES_MAX + 1;
    expect(false, true);
    prepare(PKG_FILES_MAX);
    for (unsigned i = 0; i < PKG_FILES_MAX; ++i)
        encoded.file[i].bytes = PKG_BYTES_MAX;
    expect(false, true);
    prepare(1);
    encoded.file[0].bytes = PKG_BYTES_MAX - (unsigned)available;
    expect(true, false);
    prepare(1);
    ++archive_bytes;
    expect(false, true);
    prepare(1);
    --available;
    expect(false, true);
    prepare(1);
    stat_failure = true;
    expect(false, true);
    prepare(1);
    close_failure = true;
    expect(false, true);
    prepare(1);
    encoded.identity.files = PKG_FILES_MAX + 1;
    expect(false, true);
    printf("PASS_PKG_BOUNDS limit=%u header=%zu short reads and error cleanup\n", PKG_BYTES_MAX,
           sizeof(encoded));
    return 0;
}
