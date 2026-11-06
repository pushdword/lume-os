#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int fd, duplicate;
static unsigned started, finished;

static void *transfer(void *argument)
{
    (void)argument;
    __atomic_store_n(&started, 1, __ATOMIC_RELEASE);
    for (unsigned i = 0; i < 200; ++i)
    {
        unsigned char value = (unsigned char)i, observed;
        assert(pwrite(duplicate, &value, 1, 2) == 1);
        assert(pread(duplicate, &observed, 1, 2) == 1 && observed == value);
        assert(lseek(duplicate, 0, SEEK_CUR) == 5);
    }
    __atomic_store_n(&finished, 1, __ATOMIC_RELEASE);
    return NULL;
}

int main(void)
{
    _Static_assert(sizeof(off_t) == 8, "file I/O probe requires 64-bit offsets");
    const char *path = "/tmp/file-io-probe";
    fd = open(path, O_CREAT | O_TRUNC | O_RDWR, 0600);
    assert(fd >= 3 && write(fd, "0123456789", 10) == 10);
    duplicate = dup(fd);
    assert(duplicate >= 3 && lseek(fd, 5, SEEK_SET) == 5);
    char out[8] = {0};
    assert(pread(duplicate, out, sizeof(out), 8) == 2 && !memcmp(out, "89", 2));
    assert(lseek(fd, 0, SEEK_CUR) == 5);
    assert(fcntl(fd, F_SETFL, O_APPEND) == 0);
    assert(pwrite(duplicate, "xy", 2, 0) == 2);
    assert(pread(fd, out, 2, 0) == 2 && !memcmp(out, "xy", 2));
    assert(fcntl(fd, F_SETFL, 0) == 0);
    pthread_t thread;
    assert(pthread_create(&thread, NULL, transfer, NULL) == 0);
    while (!__atomic_load_n(&started, __ATOMIC_ACQUIRE))
        assert(lseek(fd, 0, SEEK_CUR) == 5);
    while (!__atomic_load_n(&finished, __ATOMIC_ACQUIRE))
        assert(lseek(fd, 0, SEEK_CUR) == 5);
    assert(pthread_join(thread, NULL) == 0);
    int saved_input = dup(0);
    assert(saved_input >= 3 && dup2(fd, 0) == 0 && lseek(0, 0, SEEK_SET) == 0);
    assert(getchar() == 'x' && lseek(0, 0, SEEK_CUR) == 1 && getchar() == 'y');
    assert(dup2(saved_input, 0) == 0 && close(saved_input) == 0);
    assert(lseek(fd, 5, SEEK_SET) == 5 && ftruncate(duplicate, 20) == 0);
    assert(lseek(fd, 0, SEEK_CUR) == 5);
    struct stat metadata;
    assert(fstat(duplicate, &metadata) == 0 && metadata.st_size == 20);
    unsigned char extension[13];
    memset(extension, 1, sizeof(extension));
    assert(pread(fd, extension, 10, 10) == 10);
    for (unsigned i = 0; i < 10; ++i)
        assert(extension[i] == 0);
    assert(pwrite(fd, "discarded", 9, 10) == 9);
    assert(ftruncate(fd, 7) == 0 && ftruncate(duplicate, 20) == 0);
    assert(pread(fd, extension, sizeof(extension), 7) == sizeof(extension));
    for (unsigned i = 0; i < sizeof(extension); ++i)
        assert(extension[i] == 0);
    assert(lseek(duplicate, 0, SEEK_CUR) == 5);
    errno = 0;
    assert(ftruncate(fd, -1) == -1 && errno == EINVAL);
    off_t high = INT64_C(0x100001011);
    assert(lseek(fd, high, SEEK_SET) == high);
    assert(pread(duplicate, out, sizeof(out), high) == 0);
    assert(lseek(duplicate, 0, SEEK_CUR) == high);
    errno = 0;
    assert(pread(fd, out, 1, -1) == -1 && errno == EINVAL);
    assert(lseek(fd, 0, SEEK_CUR) == high);
    assert(close(duplicate) == 0 && close(fd) == 0 && unlink(path) == 0);
    puts("NATIVE_FILE_IO_PASS");
    return 0;
}
