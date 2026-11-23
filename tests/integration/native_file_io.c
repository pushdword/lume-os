#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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

struct flush_fault
{
    FILE *close_during_flush;
    int fail;
};

static ssize_t fail_flush(void *argument, const void *buffer, size_t size)
{
    (void)buffer;
    struct flush_fault *fault = argument;
    if (fault->close_during_flush)
    {
        assert(!fclose(fault->close_during_flush));
        fault->close_during_flush = NULL;
    }
    if (fault->fail)
    {
        errno = ENOSPC;
        return -1;
    }
    return (ssize_t)size;
}

static void set_allocated_buffer(FILE *file, int mode)
{
    struct __file_close *stream = (struct __file_close *)file;
    int (*close_stream)(FILE *) = stream->close;
    assert(!setvbuf(file, NULL, mode, BUFSIZ));
    assert(stream->close == close_stream);
}

static void flush_all(void)
{
    const char *first_path = "/tmp/flush-file-a";
    const char *second_path = "/tmp/flush-file-b";
    FILE *first = fopen(first_path, "w+");
    int second_fd = open(second_path, O_CREAT | O_TRUNC | O_RDWR, 0600);
    assert(first && second_fd >= 0);
    FILE *second = fdopen(second_fd, "w");
    char second_buffer[128];
    set_allocated_buffer(first, _IOFBF);
    assert(second && !setvbuf(second, second_buffer, _IOFBF, sizeof(second_buffer)));
    assert(fwrite("first", 1, 5, first) == 5);
    assert(fwrite("second", 1, 6, second) == 6);

    FILE *reader = fopen("/tmp/flush-reader", "w+");
    assert(reader && fputs("reader", reader) >= 0 && !fseek(reader, 0, SEEK_SET));
    assert(fgetc(reader) == 'r' && ungetc('Q', reader) == 'Q');
    char sample[16] = "reader";
    FILE *memory_reader = fmemopen(sample, sizeof(sample), "r+");
    assert(memory_reader && fgetc(memory_reader) == 'r' && ungetc('Q', memory_reader) == 'Q');
    char untouched[16] = "untouched";
    FILE *memory_idle = fmemopen(untouched, sizeof(untouched), "r+");
    assert(memory_idle);
    char truncated[16] = "truncate";
    FILE *memory_empty = fmemopen(truncated, sizeof(truncated), "w");
    assert(memory_empty);
    char *data = NULL;
    size_t bytes = 0;
    FILE *memory = open_memstream(&data, &bytes);
    assert(memory && fwrite("memory", 1, 6, memory) == 6 && bytes == 0);
    struct flush_fault failure = {.close_during_flush = fopen("/tmp/flush-closed", "w"), .fail = 1};
    assert(failure.close_during_flush);
    set_allocated_buffer(failure.close_during_flush, _IONBF);
    FILE *fault = funopen(&failure, NULL, fail_flush, NULL, NULL);
    assert(fault);
    set_allocated_buffer(fault, _IOFBF);
    assert(fwrite("x", 1, 1, fault) == 1);

    struct stat metadata;
    assert(fstat(second_fd, &metadata) == 0 && metadata.st_size == 0);
    errno = 0;
    assert(fflush(NULL) == EOF && errno == ENOSPC);
    assert(ferror(fault));
    assert(!failure.close_during_flush && !unlink("/tmp/flush-closed"));
    char out[8];
    assert(pread(fileno(first), out, 5, 0) == 5 && !memcmp(out, "first", 5));
    assert(pread(second_fd, out, 6, 0) == 6 && !memcmp(out, "second", 6));
    assert(bytes == 6 && !strcmp(data, "memory"));
    assert(fgetc(reader) == 'Q');
    assert(!strcmp(sample, "reader") && fgetc(memory_reader) == 'Q');
    assert(!strcmp(untouched, "untouched"));
    assert(!truncated[0]);

    failure.fail = 0;
    assert(!fclose(fault) && !fclose(memory) && !fclose(reader));
    assert(!fclose(memory_reader) && !fclose(memory_idle) && !fclose(memory_empty));
    assert(!unlink("/tmp/flush-reader"));
    free(data);
    assert(!fclose(second) && !fclose(first) && !fflush(NULL));
    assert(!unlink(first_path) && !unlink(second_path));
    puts("NATIVE_FLUSH_ALL_PASS");
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
    flush_all();
    puts("NATIVE_FILE_IO_PASS");
    return 0;
}
