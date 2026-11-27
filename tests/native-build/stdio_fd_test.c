#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static bool check_unbuffered_file(const char *path, size_t total)
{
    FILE *file = fopen(path, "rb");
    if (!file)
        return false;
    unsigned char data[BUFSIZ + 12];
    bool ok = !setvbuf(file, NULL, _IONBF, 0) && !fread(data, 0, 3, file) && ftell(file) == 0 &&
              fread(data, 5, sizeof(data) / 5, file) == total / 5 && ftell(file) == (long)total &&
              feof(file) && !ferror(file) && data[0] == 'f' && data[1] == 'd';
    for (size_t i = 2; ok && i < (total / 5) * 5; ++i)
        ok = data[i] == 0;
    ok = ok && !fseek(file, 0, SEEK_SET) && !feof(file) && fgetc(file) == 'f' &&
         ungetc('A', file) == 'A' && fread(data, 1, 3, file) == 3 && data[0] == 'A' &&
         data[1] == 'd' && data[2] == 0 && ftell(file) == 3;
    return !fclose(file) && ok;
}

static bool check_unbuffered_pipe(void)
{
    int descriptors[2];
    if (pipe(descriptors))
        return false;
    const char input[] = "abcdefg";
    ssize_t written = write(descriptors[1], input, sizeof(input) - 1);
    int closed = close(descriptors[1]);
    FILE *file = fdopen(descriptors[0], "rb");
    if (!file)
    {
        close(descriptors[0]);
        return false;
    }
    char data[9];
    bool ok = written == (ssize_t)(sizeof(input) - 1) && !closed &&
              !setvbuf(file, NULL, _IONBF, 0) && fread(data, 3, 3, file) == 2 &&
              !memcmp(data, input, 6) && feof(file) && !ferror(file);
    return !fclose(file) && ok;
}

int main(int argc, char **argv)
{
    if (fileno(stdin) != 0 || fileno(stdout) != 1 || fileno(stderr) != 2)
    {
        puts("STDIO_FD standard descriptors FAIL");
        return 1;
    }
    if (argc == 2 && !strcmp(argv[1], "--eof"))
    {
        char byte;
        if (read(0, &byte, 1) != 0)
            return 1;
        puts("STDIO_FD redirected EOF PASS");
        return 0;
    }
    if (argc != 2)
        return 2;
    FILE *file = fopen(argv[1], "w+");
    if (!file)
        return 1;
    int fd = fileno(file);
    char buffered[BUFSIZ] = {0};
    struct stat metadata;
    int ok = fd >= 3 && fd < 16 && fwrite("fd", 1, 2, file) == 2 &&
             fwrite(buffered, 1, sizeof(buffered), file) == sizeof(buffered) &&
             !fstat(fd, &metadata) && metadata.st_size == 0 && !fflush(file) &&
             !fstat(fd, &metadata) && metadata.st_size == (off_t)(2 + sizeof(buffered)) &&
             !fseek(file, 0, SEEK_SET);
    char bytes[2];
    ok = ok && fread(bytes, 1, sizeof bytes, file) == sizeof bytes && !memcmp(bytes, "fd", 2);
    ok = !fclose(file) && ok;
    ok = ok && check_unbuffered_file(argv[1], 2 + sizeof(buffered)) && check_unbuffered_pipe();
    ok = !unlink(argv[1]) && ok;
    puts(ok ? "STDIO_FD file descriptor PASS" : "STDIO_FD file descriptor FAIL");
    return ok ? 0 : 1;
}
