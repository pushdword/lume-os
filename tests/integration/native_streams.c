/* SPDX-License-Identifier: BSD-3-Clause */
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *writer(void *opaque)
{
    FILE *file = opaque;
    char data[1024];
    memset(data, 'x', sizeof(data));
    for (unsigned i = 0; i < 16; ++i)
        assert(fwrite(data, sizeof(data), 1, file) == 1);
    return opaque;
}

int main(void)
{
    char *buffer = NULL;
    size_t bytes = 99;
    FILE *file = open_memstream(&buffer, &bytes);
    assert(file && !fflush(file) && buffer && !bytes && !buffer[0]);
    assert(fileno(file) == -1 && errno == EBADF);
    assert(fprintf(file, "ação %d", 42) == 9);
    assert(!fflush(file) && bytes == 9 && !strcmp(buffer, "ação 42"));
    assert(!fseek(file, 10000, SEEK_SET) && ftell(file) == 10000);
    assert(fputc('Z', file) == 'Z' && !fflush(file) && bytes == 10001);
    for (size_t i = 9; i < 10000; ++i)
        assert(!buffer[i]);
    assert(buffer[10000] == 'Z' && !buffer[10001]);
    assert(!fseek(file, -1, SEEK_END) && fputc('Y', file) == 'Y');
    assert(!fflush(file) && bytes == 10001 && buffer[10000] == 'Y');
    assert(fseek(file, -10002, SEEK_CUR) == -1 && errno == EINVAL);
    assert(!fseek(file, 0, SEEK_SET) && fputc('A', file) == 'A');
    assert(!fflush(file) && bytes == 1 && buffer[0] == 'A');
    assert(!fclose(file) && bytes == 1 && buffer[0] == 'A');
    free(buffer);
    puts("MEMSTREAM_GROW_SEEK_CLOSE_PASS");

    file = open_memstream(&buffer, &bytes);
    assert(file);
    pthread_t threads[4];
    for (unsigned i = 0; i < 4; ++i)
        assert(!pthread_create(threads + i, NULL, writer, file));
    for (unsigned i = 0; i < 4; ++i)
    {
        void *result;
        assert(!pthread_join(threads[i], &result) && result == file);
    }
    assert(!fclose(file) && bytes == 65536 && !buffer[bytes]);
    for (size_t i = 0; i < bytes; ++i)
        assert(buffer[i] == 'x');
    free(buffer);
    puts("MEMSTREAM_CONCURRENT_PASS bytes=65536 writers=4");

    const char *path = "/data/line-native";
    file = fopen(path, "wb");
    assert(file);
    for (unsigned i = 0; i < 4096; ++i)
        assert(fputc('a', file) == 'a');
    assert(fputs("\nação\nlast", file) >= 0 && !fclose(file));
    file = fopen(path, "rb");
    assert(file);
    buffer = NULL;
    bytes = 128;
    assert(getline(&buffer, &bytes, file) == 4097 && bytes >= 4098);
    for (unsigned i = 0; i < 4096; ++i)
        assert(buffer[i] == 'a');
    assert(buffer[4096] == '\n' && !buffer[4097]);
    assert(getline(&buffer, &bytes, file) == 7 && !strcmp(buffer, "ação\n"));
    assert(getdelim(&buffer, &bytes, ':', file) == 4 && !strcmp(buffer, "last"));
    assert(getline(&buffer, &bytes, file) == -1 && feof(file));
    assert(!fclose(file));
    free(buffer);
    assert(!remove(path));
    file = fopen(path, "wb");
    assert(file && fputs("1234567\n", file) >= 0 && !fclose(file));
    file = fopen(path, "rb");
    buffer = malloc(8);
    bytes = 8;
    assert(file && buffer);
    assert(getline(&buffer, &bytes, file) == 8 && bytes >= 9);
    assert(!memcmp(buffer, "1234567\n", 9));
    assert(!fclose(file));
    free(buffer);
    assert(!remove(path));
    puts("GETDELIM_NULL_GROW_BOUNDARY_UTF8_EOF_PASS");
    puts("STREAMS_NATIVE_PASS");
    return 0;
}
